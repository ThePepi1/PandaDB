#pragma once
#include "linearStorageTraits.h"
#include "../../Data/VectorStream/includes/IStream.h"
#include <memory>
#include <string>

namespace DataStructures {

template<typename T>
class LinearStorage {
    public:
        using Traits      = LinearStorageTraits<T>;
        using FactoryType = typename Traits::FactoryType;

    private:
        size_t dim;
        size_t extra;       // e.g. Node's fixed neighbor count; 0/unused for IVector
        long long size;
        std::string filePath;
        long long getVectorOffset(long long index) const;
    public:
        LinearStorage(const std::string& filePath, size_t dim, long long size, size_t extra = 0);
        explicit LinearStorage(const std::string& filePath, bool extra = false);

        size_t getDim() const;
        long long getSize() const;
        std::string getFilePath() const;
        size_t getExtra() const ;
        LinearStorage& saveVector(const T& item, bool commit);
        LinearStorage& saveVector(const T& item);
        const LinearStorage& loadVector(long long index, T& item) const;
        LinearStorage& overwriteVector(long long index, const T& item, bool commit = true);
        std::unique_ptr<IStream<T>> getVectorStream(FactoryType& factory) const;
        std::unique_ptr<IStream<T>> getVectorStream(size_t startIndex, size_t endIndex, FactoryType& factory) const;
        std::unique_ptr<IStream<T>> getVectorStream(double proportion, FactoryType& factory) const;
        std::vector<std::unique_ptr<T>> loadBeam(
            const std::vector<long long>& indices, 
            FactoryType* factory, 
            size_t extra = 0
        ) const;
        LinearStorage& close();
    };

}