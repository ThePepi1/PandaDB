#pragma once
#include <cstddef>
#include <vector>

#include "Vector/FactoryVector.h"
#include "Vector/IVector.h"

namespace Data {
    class FlatenIndex {
        double * data;
        std::size_t num_elements;
        std::size_t vector_dimension;
        std::size_t dimension_padding;
        data::FactoryVector *factory_vector;
        public:
        FlatenIndex(std::size_t num_elements, std::size_t vector_dimension, data::FactoryVector *factory_vector = nullptr);
        ~FlatenIndex();
         std::unique_ptr<data::IVector> operator[](std::size_t index) ;
         std::size_t padding() const;
         std::size_t dimension() const;
         std::size_t elements() const;
    };
}

