#pragma once
#include <cstddef>
#include "Vector/IVector.h"
#include "Vector/FactoryVector.h"

namespace data {
    class neighbor {
        std::size_t vectorId;
        double value;
        bool memoryOwner;
        size_t dimensions;
        double *vectorData;
        public:
        neighbor();
        neighbor(size_t vectorId, double value, data::IVector* vector);
        ~neighbor();
        std::size_t getVectorId() const;
        double getValue() const;
        std::unique_ptr<IVector>  getVector(FactoryVector &factory) const;
        void swap(neighbor& second);
        void adoptOwnership();
    };
}