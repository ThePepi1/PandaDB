
#include "../includes/Sercher.h"
#include <limits>
#include <algorithm>
#include <tbb/tbb.h>
namespace algorithm {
    neighbor::~neighbor() {

    }

    neighbor::neighbor(size_t vectorId, double value, data::IVector *vector): vectorId(vectorId), value(value), vector(vector) {}

    void neighbor::swap(neighbor &second) {
        std::swap(this->vectorId, second.vectorId);
        std::swap(this->value, second.value);
        std::swap(this -> vector, second.vector);
    }

    double neighbor::getValue() const {
        return value;
    }

    std::size_t neighbor::getVectorId() const {
        return vectorId;
    }

    data::IVector *neighbor::getVector() const {
        return vector;
    }

    neighbor::neighbor(): vectorId(0), value(std::numeric_limits<double>::infinity()), vector(nullptr) {}

    neighbor* Search(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric) {
        size_t num_elements = index.elements();
        neighbor* neighborhood = new neighbor[k];
        for (size_t i = 0; i < num_elements; i++) {
            std::unique_ptr<data::IVector> element = index[i];
            double distance = metric(element.get(), &query);
            neighbor new_neighbor(i, distance, element.get());
            for (size_t j = 0; j < k; j++) {
                if (new_neighbor.getValue() < neighborhood[j].getValue()) {
                    neighborhood[j].swap(new_neighbor);
                }
            }
         }
        return neighborhood;

    }
    neighbor* SearchParrallel(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric) {
        size_t num_elements = index.elements();
        tbb::enumerable_thread_specific<std::vector<neighbor>> local_hoods(
        [&]() { return std::vector<neighbor>(k); }  );

        tbb::parallel_for(tbb::blocked_range<size_t>(0,num_elements), [&](const tbb::blocked_range<size_t> &range) {
            std::vector<neighbor>& local = local_hoods.local();
            for (size_t i = range.begin(); i < range.end(); i++) {
                std::unique_ptr<data::IVector> element = index[i];
                double distance = metric(element.get(), &query);
                neighbor new_neighbor(i, distance, element.get());
                for (size_t j = 0; j < k; j++) {
                    if (new_neighbor.getValue() < local[j].getValue()) {
                        new_neighbor.swap(local[j]);
                    }
                }
            }
        });

        neighbor* result = new neighbor[k];
        for (auto& local : local_hoods) {
            for (auto& n : local) {
                size_t worst_idx = 0;
                for (size_t j = 1; j < k; j++) {
                    if (result[j].getValue() > result[worst_idx].getValue()) {
                        worst_idx = j;
                    }
                }
                if (n.getValue() < result[worst_idx].getValue()) {
                    result[worst_idx] = n;
                }
            }
        }
        return result;
    }
}
