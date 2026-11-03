#pragma once
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
namespace encoder { 
    class IEncoder { 
        private:
        std::unordered_map<std::string, std::string> &config;
        public:
        virtual ~IEncoder() = default;
        virtual std::string_view name() const = 0;
        virtual double* operator()(std::span<const std::byte*> data) = 0;
        void setConfig(std::string name , std::string value);
    };
}