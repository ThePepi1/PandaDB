#include "../includes/plugins.h"
#include "../../Metrics/includes/IMetrics.h"
#include "../includes/MetricFactory.h"

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <mutex>



namespace {
    using CreateFn = metrics::IMetrics* (*)();
    using Creator = metrics::MetricFactory::Creator;
    void addMetrics(const std::string& name, Creator c) {
        if (!metrics::MetricFactory::instance().register_metric(name, std::move(c)))
            std::cerr << "[metrics] '" << name << "' je vec registrovana, preskacem\n";
    }
}



namespace plugins { 
    void load_plugins_metrics(const std::string& dir) {
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

        std::unique_ptr<metrics::IMetrics> probe(create());
        if (!probe) { dlclose(h); continue; }
        std::string name(probe->name());
        probe.reset();

        // h se NIKAD ne zatvara posle registracije: vtable metrikvtablee zivi u .so
        addMetrics(name, [create] { return std::unique_ptr<metrics::IMetrics>(create()); });
    }
    }

    void load_plugins_storage(const std::string& dir) {
    }

    void load_plugins_encoder(const std::string& dir) {
    }
}