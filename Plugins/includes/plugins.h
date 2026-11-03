#pragma once
#include <string>

namespace plugins { 
    void load_plugins_metrics(const std::string& dir);
    void load_plugins_storage(const std::string& dir);
    void load_plugins_encoder(const std::string& dir);
}