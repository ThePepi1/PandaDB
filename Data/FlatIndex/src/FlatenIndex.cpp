#include "../includes/FlatenIndex.h"
#include <cstdlib>
#include <new>

namespace Data {
    FlatenIndex::FlatenIndex(std::size_t num_elements, std::size_t vector_dimension, data::FactoryVector *factory_vector):num_elements(num_elements),vector_dimension(vector_dimension),factory_vector(factory_vector) {
        dimension_padding = (vector_dimension + 3) & ~3; // Align to 4
        std::size_t total_bytes = num_elements * dimension_padding * sizeof(double);
        data = (double*)std::aligned_alloc(32, total_bytes); // 32-byte alignment for AVX
        if (!data) {
            throw std::bad_alloc();
        }
    }
    FlatenIndex::~FlatenIndex() {
        std::free(data);
    }

    std::size_t FlatenIndex::padding() const {
        return dimension_padding;
    }
    std::size_t FlatenIndex::dimension() const {
        return vector_dimension;
    }
    std::size_t FlatenIndex::elements() const {
        return num_elements;
    }

    std::unique_ptr<data::IVector> FlatenIndex::operator[](std::size_t index) {
        if (index >= num_elements) {
            throw std::out_of_range("Index out of range");
        }
        double* row_ptr = data + index * dimension_padding;
        return factory_vector ? factory_vector->createVector(vector_dimension, row_ptr) : nullptr;
    }
}
