#include "../includes/Vector/FactoryVectorCPU.h"
#include "Vector/Vector_CPU.h"
namespace data {
    std::unique_ptr<IVector> FactoryVectorCPU::createVector(int dimension) {
        return std::make_unique<Vector_CPU>(dimension);
    }
    std::unique_ptr<IVector> FactoryVectorCPU::createVector(int dimension, double* external_data) {
        return std::make_unique<Vector_CPU>(dimension, external_data);
    }
    std::unique_ptr<IVector> FactoryVectorCPU::createVector(int dimension, double data) {
        return std::make_unique<Vector_CPU>(dimension, data);
    }
    std::unique_ptr<IVector> FactoryVectorCPU::createVector(int dimension, double* external_data, bool owner) {
        return std::make_unique<Vector_CPU>(dimension, external_data, owner);
    }
    std::unique_ptr<IVector> FactoryVectorCPU::createVector() {
        return std::make_unique<Vector_CPU>();
    }
}