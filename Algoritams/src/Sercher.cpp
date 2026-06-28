
#include "../includes/Sercher.h"
#include <limits>
#include <algorithm>
#include <tbb/tbb.h>
namespace algorithm {
    
    data::neighbor* Search(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric) {
        size_t num_elements = index.elements();
        data::neighbor* neighborhood = new data::neighbor[k];
        for (size_t i = 0; i < num_elements; i++) {
            std::unique_ptr<data::IVector> element = index[i];
            double distance = metric(element.get(), &query);
            data::neighbor new_neighbor(i, distance, element.get());
            for (size_t j = 0; j < k; j++) {
                if (new_neighbor.getValue() < neighborhood[j].getValue()) {
                    neighborhood[j].swap(new_neighbor);
                }
            }
         }
        for (size_t i = 0; i < k; i++) {
            neighborhood[i].adoptOwnership();
        }
        return neighborhood;

    }

    data::neighbor* SearchParrallel(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric) {
        size_t num_elements = index.elements();
        tbb::enumerable_thread_specific<std::vector<data::neighbor>> local_hoods(
        [&]() { return std::vector<data::neighbor>(k); }  );

        tbb::parallel_for(tbb::blocked_range<size_t>(0,num_elements), [&](const tbb::blocked_range<size_t> &range) {
            std::vector<data::neighbor>& local = local_hoods.local();
            for (size_t i = range.begin(); i < range.end(); i++) {
                std::unique_ptr<data::IVector> element = index[i];
                double distance = metric(element.get(), &query);
                data::neighbor new_neighbor(i, distance, element.get());
                for (size_t j = 0; j < k; j++) {
                    if (new_neighbor.getValue() < local[j].getValue()) {
                        new_neighbor.swap(local[j]);
                    }
                }
            }
        });

        data::neighbor* result = new data::neighbor[k];
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
        for (size_t i = 0; i < k; i++) {
            result[i].adoptOwnership();
        }
        return result;
    }
}
