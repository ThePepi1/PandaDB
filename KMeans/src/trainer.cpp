#include "../includes/trainer.h"
#include "Vector/IVector.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <random>
namespace KMeans{ 
    void Trainer::train(CodeBook& codeBook, DataStructures::IVectorStream& stream,
                    const metrics::IMetrics& metrics, int maxIterations) {
    const size_t reservoirSize = 4096;
    std::mt19937_64 rng{std::random_device{}()};

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        stream.reset();

        std::vector<std::unique_ptr<data::IVector>> reservoir;
        reservoir.reserve(reservoirSize);
        size_t seen = 0;

        while (stream.hasNext()) {
            auto point = stream.next();
            codeBook.fit(*point, metrics);

            // reservoir sampling: clone samo kad je tacka stvarno prihvacena
            ++seen;
            if (reservoir.size() < reservoirSize) {
                reservoir.push_back(point->clone(0, point->Dimension()));
            } else {
                std::uniform_int_distribution<size_t> d(0, seen - 1);
                size_t j = d(rng);
                if (j < reservoirSize) {
                    reservoir[j] = point->clone(0, point->Dimension());
                }
            }
        }

        if (!codeBook.updateAverages(reservoir, metrics)) {
            break; // nema promena -> konvergencija
        }
    }
}

void Trainer::train(ProductQuantizer& pq, DataStructures::IVectorStream& stream,
                    const metrics::IMetrics& metrics, int maxIterations) {
    const int numSubspaces = pq.getNumSubspaces();
    const int dim = pq.getDim();
    const int subDim = dim / numSubspaces;

    const size_t reservoirSize = 4096;
    std::mt19937_64 rng{std::random_device{}()};

    std::vector<bool> continueTraining(numSubspaces, true);

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        stream.reset();

        std::vector<std::unique_ptr<data::IVector>> reservoir;
        reservoir.reserve(reservoirSize);
        size_t seen = 0;

        while (stream.hasNext()) {
            auto point = stream.next();

            for (int i = 0; i < numSubspaces; ++i) {
                if (!continueTraining[i]) {
                    continue;
                }
                const int startIdx = i * subDim;
                const int endIdx = (i == numSubspaces - 1) ? dim : startIdx + subDim;

                std::unique_ptr<data::IVector> subVector =
                    pq.factory.createVector(endIdx - startIdx, (*point)[startIdx]);
                pq.codeBooks[i]->fit(*subVector, metrics);
            }

            ++seen;
            if (reservoir.size() < reservoirSize) {
                reservoir.push_back(point->clone(0 , point->Dimension()));
            } else {
                std::uniform_int_distribution<size_t> d(0, seen - 1);
                size_t j = d(rng);
                if (j < reservoirSize) {
                    reservoir[j] = point->clone(0 , point->Dimension());
                }
            }
        }

        bool allConverged = true;
        for (int i = 0; i < numSubspaces; ++i) {
            if (continueTraining[i]) {
                const int startIdx = i * subDim;
                const int endIdx = (i == numSubspaces - 1) ? dim : startIdx + subDim;

                // kandidati za reseed secemo na dimenzije ovog potprostora
                std::vector<std::unique_ptr<data::IVector>> subCandidates;
                subCandidates.reserve(reservoir.size());
                for (const auto& p : reservoir) {
                    subCandidates.push_back(pq.factory.createVector(endIdx - startIdx, (*p)[startIdx]));
                }

                continueTraining[i] = pq.codeBooks[i]->updateAverages(subCandidates, metrics);
            }
            allConverged = allConverged && !continueTraining[i];
        }

        if (allConverged) {
            break;
        }
    }
}
  void Trainer::train(ProductQuantizer& pq, CodeBook& codeBook, DataStructures::IVectorStream& stream,
                    const metrics::IMetrics& metrics, int maxIterations) {
    const int numSubspaces = pq.getNumSubspaces();
    const int dim = pq.getDim();
    const int subDim = dim / numSubspaces;

    std::vector<bool> continueTraining(numSubspaces, true);
    bool continueCodeBookTraining = true;

    const size_t batchSize = 50000;
    const size_t reservoirSize = 4096;
    std::mt19937_64 rng{std::random_device{}()};

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        auto startTime = std::chrono::high_resolution_clock::now();
        stream.reset();
        std::vector<std::unique_ptr<data::IVector>> reservoir;
        reservoir.reserve(reservoirSize);
        size_t seen = 0;

        while (stream.hasNext()) {
            std::vector<std::shared_ptr<data::IVector>> batch = stream.getVectors(batchSize);
            if (batch.empty()) {
                break;
            }

            tbb::parallel_for(tbb::blocked_range<size_t>(0, batch.size()),
                [&](const tbb::blocked_range<size_t>& range) {
                    for (size_t index = range.begin(); index < range.end(); ++index) {
                        const auto& point = batch[index];

                        for (int i = 0; i < numSubspaces; ++i) {
                            if (!continueTraining[i]) {
                                continue;
                            }
                            const int startIdx = i * subDim;
                            const int endIdx = (i == numSubspaces - 1) ? dim : startIdx + subDim;

                            std::unique_ptr<data::IVector> subVector =
                                pq.factory.createVector(endIdx - startIdx, (*point)[startIdx]);
                            pq.codeBooks[i]->fit(*subVector, metrics);
                        }

                        if (continueCodeBookTraining) {
                            codeBook.fit(*point, metrics);
                        }
                    }
                });

            for (const auto& p : batch) {
                ++seen;
                if (reservoir.size() < reservoirSize) {
                    reservoir.push_back(p->clone(0, p->Dimension()));
                } else {
                    std::uniform_int_distribution<size_t> d(0, seen - 1);
                    size_t j = d(rng);
                    if (j < reservoirSize) {
                        reservoir[j] = p->clone(0, p->Dimension());
                    }
                }
            }
        }

        for (int i = 0; i < numSubspaces; ++i) {
            if (!continueTraining[i]) {
                continue;
            }
            const int startIdx = i * subDim;
            const int endIdx = (i == numSubspaces - 1) ? dim : startIdx + subDim;

            std::vector<std::unique_ptr<data::IVector>> subCandidates;
            subCandidates.reserve(reservoir.size());
            for (const auto& p : reservoir) {
                subCandidates.push_back(pq.factory.createVector(endIdx - startIdx, (*p)[startIdx]));
            }
            continueTraining[i] = pq.codeBooks[i]->updateAverages(subCandidates, metrics);
        }

        if (continueCodeBookTraining) {
            continueCodeBookTraining = codeBook.updateAverages(reservoir, metrics);
        }

        bool allConverged = !continueCodeBookTraining;
        for (int i = 0; i < numSubspaces; ++i) {
            allConverged = allConverged && !continueTraining[i];
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        std::cout << "Iteration " << iteration + 1 << " completed in " << elapsed.count() << " seconds." << std::endl;

        if (allConverged) {
            break;
        }
    }
                    
}
}