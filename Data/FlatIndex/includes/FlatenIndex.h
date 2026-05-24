#pragma once
#include <cstddef>

namespace Data {
    class FlatenIndex {
        double * data;
        std::size_t num_elements;
        std::size_t vector_dimension;
        std::size_t dimension_padding;
        public:
        FlatenIndex(std::size_t num_elements, std::size_t vector_dimension);
        ~FlatenIndex();
         double* operator[](std::size_t index) ;
         std::size_t padding() const;
         std::size_t dimension() const;
         std::size_t elements() const;
    };
}

