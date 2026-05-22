#pragma once
#include "Vector/IVector.h"

namespace metrics {
    class IMetrics {
        public:
        virtual ~IMetrics() = default;
        virtual double operator()(data::IVector *first, data::IVector *second) const = 0;
        virtual std::string_view name() const = 0;

    };
}
