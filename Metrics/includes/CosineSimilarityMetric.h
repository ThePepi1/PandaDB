//
// Created by pepi on 5/21/26.
//
#pragma once
#include "IMetrics.h"

namespace metrics {
    class CosineSimilarityMetric: public IMetrics {
    public:
        CosineSimilarityMetric();
        virtual ~CosineSimilarityMetric() = default;
        double operator()(const data::IVector &first, const data::IVector &second) const override;
        std::string_view name() const override;
    };
}

