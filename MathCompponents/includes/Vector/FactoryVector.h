#pragma once

#include "IVector.h"
namespace data {
    class FactoryVector {
        public:
        FactoryVector() = default;
        virtual std::unique_ptr<IVector> createVector(int dimension) = 0;
        virtual std::unique_ptr<IVector> createVector(int dimension, double* external_data) = 0;
        virtual std::unique_ptr<IVector> createVector(int dimension, double data) = 0;
    };
}
