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
		auto dot_product = first * second;
		auto norm_product = first.norm() * second.norm();
		if (norm_product == 0) {
			std::cerr << "Warning: One of the vectors has zero norm. Returning 0" << std::endl;
			return 0.0;
			}
        return (first * second )/ ((first.norm()) * (second.norm()));
    }

}
