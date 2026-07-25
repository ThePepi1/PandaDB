#pragma once
#include "IMetrics.h"

namespace metrics {
    class EuclideMetrics: public IMetrics {
        public:
        EuclideMetrics();
        virtual ~EuclideMetrics() = default;
        double operator()(const data::IVector &first, const data::IVector &second) const override;
        std::string_view name() const override;
    };
}

