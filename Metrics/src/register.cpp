// register.cpp
#include "../includes/register.h"
#include "../includes/MetricFactory.h"
#include "../includes/EuclideMetrics.h"
#include "../includes/CosineSimilarityMetric.h"
#include "../includes/DotMetric.h"

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <mutex>

namespace metrics {

namespace {
    using CreateFn = IMetrics* (*)();
    using Creator = MetricFactory::Creator;
    void add(const std::string& name, Creator c) {
        if (!MetricFactory::instance().register_metric(name, std::move(c)))
            std::cerr << "[metrics] '" << name << "' je vec registrovana, preskacem\n";
    }
}

void register_builtin_metrics() {
    static std::once_flag once;
    std::call_once(once, [] {
        add("euclide",           [] { return std::make_unique<EuclideMetrics>(); });
        add("cosine_similarity", [] { return std::make_unique<CosineSimilarityMetric>(); });
        add("dot_product",       [] { return std::make_unique<DotMetric>(); });
    });
}
}