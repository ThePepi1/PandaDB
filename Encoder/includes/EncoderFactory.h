#pragma once
#include <functional>
#include "IEncoder.h"
#include <memory>
#include <string>
#include <unordered_map>
namespace encoder { 
    class EncoderFactory { 

        public:
            using Creator = std::function<std::unique_ptr<IEncoder>()>;
            using CreatorMap = std::function<std::unique_ptr<IEncoder>(std::unordered_map<std::string,std::string> &params)>;
            using Help = std::function<std::string()>;
            static EncoderFactory& instance();
            
            std::unique_ptr<IEncoder> create_encoder(const std::string& name);
            std::unique_ptr<IEncoder> create_encoder(const std::string& name,
                 std::unordered_map<std::string,std::string> &params);
            std::string get_help(const std::string& name);
            bool register_encoder(const std::string& name, Creator creator, CreatorMap creator_map, Help help);

        private:
            std::unordered_map<std::string, Creator> creators_;
            std::unordered_map<std::string, CreatorMap> creators_map_;
            std::unordered_map<std::string, Help> help_;   
    };
}