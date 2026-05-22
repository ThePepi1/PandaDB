
#include "../includes/register.h"
#include "../includes/MetricFactory.h"
#include "../includes/EuclideMetrics.h"
#include "../includes/CosineSimilarityMetric.h"
#include "../includes//DotMetric.h"
#include "../includes/register.h"

namespace metrics {

     void register_builtin_metrics() {
        MetricFactory::instance().register_metric(
            "euclide",
            [] {
                return std::make_unique<EuclideMetrics>();
            }
        );
         MetricFactory::instance().register_metric(
             "cosine_similarity",
             [] {
                 return std::make_unique<CosineSimilarityMetric>();
             }
         );
         MetricFactory::instance().register_metric(
             "dot_product",
             [] {
                 return std::make_unique<DotMetric>();
             }
         );
    }

}