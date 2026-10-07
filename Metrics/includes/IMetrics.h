#pragma once
#include "Vector/IVector.h"
#include <vector>

namespace KMeans{
    class ProductQuantizer;
}
namespace metrics {
    class IMetrics {
        public:
        virtual ~IMetrics() = default;
        virtual double operator()(const data::IVector &first, const data::IVector &second) const = 0;
        virtual std::string_view name() const = 0;
        double distance(std::vector<long long> vectorSegment, const KMeans::ProductQuantizer &quantizaer, const data::IVector &first);
    };
}
