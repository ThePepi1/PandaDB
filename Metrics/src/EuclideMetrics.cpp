//
// Created by pepi on 5/8/26.
//

#include "../includes/EuclideMetrics.h"

#include <cmath>
#include <math.h>

#include "../includes/MetricFactory.h"

namespace metrics {
    EuclideMetrics::EuclideMetrics() = default;

    std::string_view EuclideMetrics::name() const {
        return "euclide";
    }

    double EuclideMetrics::operator()(data::IVector *first, data::IVector *second) const {
        if (first->Dimension() != second->Dimension()) {
            throw std::invalid_argument("Vectors must be of the same size");
        }
        double sum = 0.0;

        for (size_t i = 0; i < first->Dimension(); i++) {
            double diff = (*first)(i) - (*second)(i);
            sum += diff * diff;
        }
        return sqrt(sum);
    }

}
namespace {

    struct EuclideMetricRegister {
        EuclideMetricRegister() {
            metrics::MetricFactory::instance().register_metric(
                "euclide",
                [] {
                    return std::make_unique<metrics::EuclideMetrics>();
                }
            );
        }
    };

    [[maybe_unused]]
    static EuclideMetricRegister reg_euclide;

}
