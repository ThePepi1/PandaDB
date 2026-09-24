#pragma once

#include "FactoryVector.h"
#include "IVector.h"
namespace data {
    class FactoryVectorCPU: public FactoryVector{
    public:
        FactoryVectorCPU() = default;
        std::unique_ptr<IVector> createVector(int dimension) override;
        std::unique_ptr<IVector> createVector(int dimension, double* external_data) override;
        std::unique_ptr<IVector> createVector(int dimension, double data) override;
        std::unique_ptr<IVector> createVector(int dimension, double* external_data, bool owner) override;
        std::unique_ptr<IVector> createVector() override;
    };
}


