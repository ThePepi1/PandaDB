//
// Created by pepi on 5/21/26.
//
#include <iostream>
#include "../includes/CosineSimilarityMetric.h"
namespace metrics {
    CosineSimilarityMetric::CosineSimilarityMetric() = default;
    std::string_view CosineSimilarityMetric::name() const {
        return "cosine_similarity";
    }
    double CosineSimilarityMetric::operator()(const data::IVector &first, const data::IVector &second) const {
        return (first * second )/ ((first.norm()) * (second.norm()));
    }

}
