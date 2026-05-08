
#include "../includes/register.h"
#include "../includes/MetricFactory.h"
#include "../includes/EuclideMetrics.h"
#include "../includes/register.h"

namespace metrics {

     void register_builtin_metrics() {
        MetricFactory::instance().register_metric(
            "euclide",
            [] {
                return std::make_unique<EuclideMetrics>();
            }
        );
    }

}