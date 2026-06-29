#include "../includes/neighbor.h"
namespace data {

    neighbor::neighbor() : vectorId(0), value(std::numeric_limits<double>::max()), memoryOwner(false), dimensions(0), vectorData(nullptr) {}

    neighbor::neighbor(size_t vectorId, double value, data::IVector* vector)
        : vectorId(vectorId), value(value), memoryOwner(vector->Owner()), dimensions(vector->Dimension()), vectorData(vector->Data()){
        if (vectorData == nullptr) {
            throw std::invalid_argument("Vector data cannot be null");
        }
        vector->Disown();  // Disown the vector to avoid double deletion
    }
    neighbor::~neighbor() {
        if (memoryOwner && vectorData) {
            delete[] vectorData;
        }
    }

    std::size_t neighbor::getVectorId() const {
        return vectorId;
    }

    double neighbor::getValue() const {
        return value;
    }

    std::unique_ptr<IVector> neighbor::getVector(FactoryVector& factory_vector) const {
        if (vectorData == nullptr) {
            throw std::runtime_error("Vector data is null");
        }
        return factory_vector.createVector(dimensions, vectorData);
    }

    void neighbor::swap(neighbor& second) {
        std::swap(vectorId, second.vectorId);
        std::swap(value, second.value);
        std::swap(memoryOwner, second.memoryOwner);
        std::swap(dimensions, second.dimensions);
        std::swap(vectorData, second.vectorData);
    }

    void neighbor::adoptOwnership() {
        if (memoryOwner) {
            return;
        }
        memoryOwner = true;
        double *data = new double[dimensions];
        for (std::size_t i = 0; i < dimensions; ++i) {
            data[i] = vectorData[i];
        }
        vectorData = data;
    }
}
