#include <cstddef>

#include "../../Data/FlatIndex/includes/FlatenIndex.h"
#include "../../Metrics/includes/EuclideMetrics.h"
#include "neighbor.h"
#pragma once
namespace algorithm {


    data::neighbor* Search(DataStructures::FlatenIndex& index, data::IVector& query, size_t k, metrics::IMetrics& metric);
    data::neighbor* SearchParrallel(DataStructures::FlatenIndex &index, data::IVector &query, size_t k, metrics::IMetrics &metric);
}
