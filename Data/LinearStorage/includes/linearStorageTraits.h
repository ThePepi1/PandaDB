#pragma once
#include <cstddef>
#include <memory>
#include "Vector/IVector.h"
#include "Vector/FactoryVector.h"
#include "../../Data/Vamana/includes/Node.h"

namespace DataStructures {


    template<typename T>
    struct LinearStorageTraits;

    template<>
    struct LinearStorageTraits<data::IVector> {
        using FactoryType = data::FactoryVector;
        using Handle = std::shared_ptr<data::IVector>;

        static size_t recordSize(size_t dim, size_t /*extra*/ = 0) {
            return data::IVector::getSizeInBytes(dim);
        }

        static Handle createEmpty(FactoryType& factory, size_t dim, size_t /*extra*/ = 0) {
            return factory.createVector(dim);
        }
    };


    template<>
    struct LinearStorageTraits<Node> {
        using FactoryType = data::FactoryVector;
        using Handle = std::shared_ptr<Node>;

        static size_t recordSize(size_t dim, size_t neighborsSize) {
            return Node::getSerializedSize(dim, neighborsSize);
        }        
        static Handle createEmpty(FactoryType& factory, size_t /*dim*/, size_t neighborsSize) {
            return std::make_shared<Node>(neighborsSize, factory);
        }
    };

}