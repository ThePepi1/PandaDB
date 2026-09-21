#pragma once 
#include "../includes/Vector/IVector.h"
#include <vector>
namespace DataStructures { 
    class IVectorStream {
        private:
        protected:
        public:
            virtual ~IVectorStream() = default;
            virtual std::shared_ptr<data::IVector> getVector() = 0;
            virtual std::vector<std::shared_ptr<data::IVector>> getVectors(size_t count) = 0;
            
    };
}