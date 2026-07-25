//
// Created by pepi on 5/21/26.
//
#pragma once
#include "IMetrics.h"
namespace metrics {
    class DotMetric : public IMetrics {
        public:
        DotMetric();
        virtual ~DotMetric();
        double operator()(const data::IVector &first, const data::IVector &second) const override;
        std::string_view name() const override;
    };
}

