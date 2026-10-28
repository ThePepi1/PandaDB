#pragma once
#include <string>
namespace metrics {
    void register_builtin_metrics();
    void load_metric_plugins(const std::string& dir);

}

