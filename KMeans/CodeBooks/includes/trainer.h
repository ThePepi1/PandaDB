#pragma once
#include "codeBook.h"
#include "productquntizier.h"

namespace KMeans {
    class Trainer { 
        public:
            void train(CodeBook& codeBook, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations);
            void train(ProductQuantizer& pq, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations);
            void train(ProductQuantizer& pq, CodeBook& codeBook, DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations);
    };

}