#include "../includes/FlatenIndex.h"
#include <cstdlib>
#include <new>

namespace Data {
    FlatenIndex::FlatenIndex(std::size_t num_elements, std::size_t vector_dimension):num_elements(num_elements),vector_dimension(vector_dimension) {
        dimension_padding = (vector_dimension + 3) & ~3; // Align to 4
        std::size_t total_bytes = num_elements * dimension_padding * sizeof(double);
        data = (double*)std::aligned_alloc(64, total_bytes); // 32-byte alignment for AVX
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

    double *FlatenIndex::operator[](std::size_t index) {
        return data + index * dimension_padding;
    }
}
