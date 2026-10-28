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

void load_metric_plugins(const std::string& dir) {
    namespace fs = std::filesystem;
    if (!fs::is_directory(dir)) return;

    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.path().extension() != ".so") continue;

        void* h = dlopen(e.path().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!h) {
            std::cerr << "[metrics] " << e.path() << ": " << dlerror() << "\n";
            continue;
        }
        auto create = reinterpret_cast<CreateFn>(dlsym(h, "panda_create_metric"));
        if (!create) {
            std::cerr << "[metrics] " << e.path() << ": nema panda_create_metric\n";
            dlclose(h);   
            continue;
        }

        std::unique_ptr<IMetrics> probe(create());
        if (!probe) { dlclose(h); continue; }
        std::string name(probe->name());
        probe.reset();

        // h se NIKAD ne zatvara posle registracije: vtable metrikvtablee zivi u .so
        add(name, [create] { return std::unique_ptr<IMetrics>(create()); });
    }
}

void register_builtin_metrics() {
    static std::once_flag once;
    std::call_once(once, [] {
        add("euclide",           [] { return std::make_unique<EuclideMetrics>(); });
        add("cosine_similarity", [] { return std::make_unique<CosineSimilarityMetric>(); });
        add("dot_product",       [] { return std::make_unique<DotMetric>(); });

        const char* env = std::getenv("METRIC_PLUGIN_DIR");
        load_metric_plugins(env ? env : "/plugins");
    });
}

}