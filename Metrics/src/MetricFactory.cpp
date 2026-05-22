//
// Created by pepi on 5/8/26.
//

#include "../includes/MetricFactory.h"

namespace metrics {
    MetricFactory &MetricFactory::instance() {
        static MetricFactory factory;
        return factory;
    }

    bool MetricFactory::register_metric(const std::string& name, Creator creator) {
        if (creators_.find(name) != creators_.end()) {
            return false;
        }
        creators_[std::move(name)] = std::move(creator);
        return true;
    }

    std::unique_ptr<IMetrics> MetricFactory::create_metric(const std::string& name) {
        auto it = creators_.find(name);
        if (it == creators_.end()) {
            return nullptr;
        }
        return it->second();
    }
}
