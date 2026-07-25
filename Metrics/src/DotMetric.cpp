//
// Created by pepi on 5/21/26.
//

#include "../includes/DotMetric.h"
namespace metrics {
    DotMetric::DotMetric() = default;
    DotMetric::~DotMetric() = default;
    std::string_view DotMetric::name() const {
        return "dot_product";
    }
    double DotMetric::operator()(const data::IVector &first, const data::IVector &second) const {
        return (first) * (second);
    }

}