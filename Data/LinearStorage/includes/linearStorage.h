#pragma once
#include "Vector/FactoryVector.h"
#include "Vector/IVector.h"
#include "../../Data/VectorStream/includes/IVectorStream.h"
#include <memory>
#include <string>
namespace DataStructures {
class LinearStorage { 
    private:
        size_t dim;
        long long size;
        std::string filePath;
        long long getVectorOffset(long long index) const;
    public:
        LinearStorage(const std::string& filePath, size_t dim, long long size);
        LinearStorage(const std::string& filePath);
        size_t getDim() const;
        long long getSize() const;
        std::string getFilePath() const;
        LinearStorage& saveVector(const data::IVector& vector , bool commit);
        LinearStorage& saveVector(const data::IVector& vector);
        const LinearStorage& loadVector(long long index, data::IVector& vector) const;
        std::unique_ptr<IVectorStream> getVectorStream(double proportion, data::FactoryVector & factory) const;
        std::unique_ptr<IVectorStream> getVectorStream(size_t startIndex, size_t endIndex, data::FactoryVector & factory) const;
        std::unique_ptr<IVectorStream> getVectorStream(data::FactoryVector & factory) const;
    };
}