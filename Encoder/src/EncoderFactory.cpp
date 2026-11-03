#include "../includes/EncoderFactory.h"

namespace encoder {
    
    EncoderFactory& EncoderFactory::instance() {
        static EncoderFactory instance;
        return instance;
    }

    std::unique_ptr<IEncoder> EncoderFactory::create_encoder(const std::string& name) {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        return nullptr;
    }

    std::unique_ptr<IEncoder> EncoderFactory::create_encoder(const std::string& name,
                 std::unordered_map<std::string,std::string> &params) {
        auto it = creators_map_.find(name);
        if (it != creators_map_.end()) {
            return it->second(params);
        }
        return nullptr;
    }

    std::string EncoderFactory::get_help(const std::string& name) {
        auto it = help_.find(name);
        if (it != help_.end()) {
            return it->second();
        }
        return "No help available for this encoder.";
    }

    bool EncoderFactory::register_encoder(const std::string& name, Creator creator, CreatorMap creator_map, Help help) {
        creators_[name] = creator;
        creators_map_[name] = creator_map;
        help_[name] = help;
        return true;
    }
}