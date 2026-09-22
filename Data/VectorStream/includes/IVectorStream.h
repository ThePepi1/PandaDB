#pragma once 
#include "../includes/Vector/IVector.h"
#include <vector>
namespace DataStructures { 
    class IVectorStream {
        private:
        protected:
        public:
            virtual ~IVectorStream() = default;
            virtual std::vector<std::shared_ptr<data::IVector>> getVectors(size_t count) = 0;
            virtual bool hasNext() = 0;
            virtual std::shared_ptr<data::IVector> next() = 0; 
            virtual void reset() = 0;
            virtual int getDim() const = 0;
    };
}