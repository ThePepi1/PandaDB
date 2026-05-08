#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
#include <string>

#include "IMetrics.h"

namespace metrics {

    class MetricFactory {
    public:
        using Creator = std::function<std::unique_ptr<IMetrics>()>;

        static MetricFactory& instance();

        bool register_metric(const std::string& name, Creator creator);

        std::unique_ptr<IMetrics> create_metric(const std::string& name);

    private:
        std::unordered_map<std::string, Creator> creators_;
    };

}