#include "../includes/trainer.h"
#include "Vector/IVector.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
namespace KMeans{ 
    void Trainer::train(CodeBook& codeBook, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations) {
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            stream.reset();
            data::IVector* point;
            while (stream.hasNext()) {
                auto point = stream.next();
                codeBook.fit(*point, metrics);
            }
            if (!codeBook.updateAverages()) {
                break; // No changes in centroids, convergence reached
            }
        }
    }
    void Trainer::train(ProductQuantizer& pq, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations) {
        bool continueTraining[pq.getNumSubspaces()];
        for (int i = 0; i < pq.getNumSubspaces(); ++i) {
            continueTraining[i] = true;
        }
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
          stream.reset();
          bool allConverged = true;
          while(stream.hasNext()) { 
            auto point = stream.next();
            for (int i = 0; i < pq.getNumSubspaces(); ++i) {
                if (continueTraining[i]) {
                    int startIdx = i * (pq.getDim() / pq.getNumSubspaces());
                    int endIdx = (i == pq.getNumSubspaces() - 1) ? pq.getDim() : startIdx + (pq.getDim() / pq.getNumSubspaces());
                    // Can do it bcs of the ownership
                    std::unique_ptr<data::IVector> subVector = pq.factory.createVector(endIdx - startIdx, (*point)[startIdx]);
                    pq.codeBooks[i]->fit(*subVector, metrics);
                }

            }
          }
          for (int i = 0; i < pq.getNumSubspaces(); ++i) {
                if (continueTraining[i]) {
                    continueTraining[i] = pq.codeBooks[i]->updateAverages();
                }
                allConverged = allConverged && !continueTraining[i];
            }
            if (allConverged) {
                break; // All codebooks have converged
            }
        }
    }   
   void Trainer::train(ProductQuantizer& pq, CodeBook& codeBook, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations) {
    const int numSubspaces = pq.getNumSubspaces();
    const int dim = pq.getDim();

    std::vector<bool> continueTraining(numSubspaces, true);
    bool continueCodeBookTraining = true;

    const size_t batchSize = 50000;

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        auto startTime = std::chrono::high_resolution_clock::now();
        stream.reset();

        while (stream.hasNext()) {
            std::vector<std::shared_ptr<data::IVector>> batch = stream.getVectors(batchSize);

            if (batch.empty()) {
                break;
            }

            tbb::parallel_for(tbb::blocked_range<size_t>(0, batch.size()), [&](const tbb::blocked_range<size_t>& range) {
                for (size_t index = range.begin(); index < range.end(); ++index) {
                    const auto& point = batch[index];

                    for (int i = 0; i < numSubspaces; ++i) {
                        if (!continueTraining[i]) {
                            continue;
                        }

                        const int startIdx = i * (dim / numSubspaces);
                        const int endIdx = (i == numSubspaces - 1) ? dim : startIdx + (dim / numSubspaces);

                        std::unique_ptr<data::IVector> subVector = pq.factory.createVector(endIdx - startIdx, (*point)[startIdx]);
                        pq.codeBooks[i]->fit(*subVector, metrics);
                    }

                    if (continueCodeBookTraining) {
                        codeBook.fit(*point, metrics);
                    }
                }
            });
        }

        for (int i = 0; i < numSubspaces; ++i) {
            if (continueTraining[i]) {
                continueTraining[i] = pq.codeBooks[i]->updateAverages();
            }
        }

        if (continueCodeBookTraining) {
            continueCodeBookTraining = codeBook.updateAverages();
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