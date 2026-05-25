#include <cstddef>

#include "../../Data/FlatIndex/includes/FlatenIndex.h"
#include "../../Metrics/includes/EuclideMetrics.h"

namespace algorithm {

    class neighbor {
        std::size_t vectorId;
        double value;
        data::IVector* vector;
        public:
        neighbor();
        neighbor(size_t vectorId, double value, data::IVector* vector);
        ~neighbor();
        std::size_t getVectorId() const;
        double getValue() const;
        data::IVector*  getVector() const;
        void swap(neighbor& second);
    };
    neighbor* Search(DataStructures::FlatenIndex& index, data::IVector& query, size_t k, metrics::IMetrics& metric);
    neighbor* SearchParrallel(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric);
}
