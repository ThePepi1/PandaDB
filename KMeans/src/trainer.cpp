#include "../includes/trainer.h"
#include "Vector/IVector.h"

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
        bool continueTraining[pq.getNumSubspaces()];
        for (int i = 0; i < pq.getNumSubspaces(); ++i) {
                continueTraining[i] = true;
            }
        bool continueCodeBookTraining = true;
        
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            stream.reset();
            bool allConverged = true;
            while(stream.hasNext()) {
                auto point = stream.next();
                for (int i = 0; i < pq.getNumSubspaces(); ++i) {
                    if (continueTraining[i]) {
                        int startIdx = i * (pq.getDim() / pq.getNumSubspaces());
                        int endIdx = (i == pq.getNumSubspaces() - 1) ? pq.getDim() : startIdx + (pq.getDim() / pq.getNumSubspaces());
                        std::unique_ptr<data::IVector> subVector = pq.factory.createVector(endIdx - startIdx, (*point)[startIdx]);
                        pq.codeBooks[i]->fit(*subVector, metrics);
                    }

                }
                if (continueCodeBookTraining) {
                    codeBook.fit(*point, metrics);
                }
        }
            for (int i = 0; i < pq.getNumSubspaces(); ++i) {
                if (continueTraining[i]) {
                    continueTraining[i] = pq.codeBooks[i]->updateAverages();
                }
                allConverged = allConverged && !continueTraining[i];
            }
            if (continueCodeBookTraining) {
                continueCodeBookTraining = codeBook.updateAverages();
            }
            allConverged = allConverged && !continueCodeBookTraining;
            if (allConverged) {
                break; // All codebooks and the main codebook have converged
            }
        }
    }
}