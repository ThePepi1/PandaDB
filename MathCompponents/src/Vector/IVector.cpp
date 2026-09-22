#include "../../includes/Vector/IVector.h"
#include <stdexcept>
#include <iostream>

namespace data {
    IVector::IVector(std::size_t dimension): dimension(dimension) {
        this->data = new double[dimension];
    }
    IVector::IVector(std::size_t dimension, double* external_data, bool owner): dimension(dimension), data(external_data), owner(owner) {}
    IVector::IVector(std::size_t dimension, double *external_data) : dimension(dimension), data(external_data), owner(false){}
    IVector::IVector(): dimension(0), data(nullptr), owner(true) {}
    IVector::IVector(std::size_t dimension, double data):dimension(dimension) {
        this->data = new double[dimension];
        for (size_t i = 0; i < dimension; i++)
            this->data[i] = data;
    }

    IVector::~IVector() {
        if (this->data != nullptr && owner)
            delete[] this->data;
    }

    double *IVector::Data() const {
        return this->data;
    }

    size_t IVector::Dimension() const {
        return this->dimension;
    }

    double &IVector::operator()(std::size_t index) const {
        if (index >= dimension)
            throw std::out_of_range("Index out of bounds");

        return data[index];
    }

    double *IVector::operator[](std::size_t index) const {
        if (index >= dimension)
            throw std::out_of_range("Index out of bounds");

        return &data[index];
    }
    std::ostream& operator<<(std::ostream& stream, IVector& vector) {
        for (size_t i = 0; i < vector.dimension; i++) {
            stream << vector.Data()[i] << " ";
        }
        return stream;
    }
    std::istream& operator>>(std::istream& stream, IVector& vector) {
        for (size_t i = 0; i < vector.dimension; i++) {
            stream >> vector(i);
        }
        return stream;
    }
    bool IVector::Owner() const {
        return this->owner;
    }
    void IVector::Disown() {
        this->owner = false;
    }
    file::fileReaderWriter& operator<<(file::fileReaderWriter& stream, const IVector& vector) {
        stream << vector.dimension; 
        for (size_t i = 0; i < vector.dimension; i++) {
            stream << vector.Data()[i];
        }
        return stream;
    }
    file::fileReader& operator>>(file::fileReader& stream, IVector& vector) {
        size_t dimension;
        stream >> dimension; 
        vector.dimension = dimension;
        if (vector.data != nullptr && vector.owner) {
            delete[] vector.data;
        }
        vector.data = new double[dimension];
        for (size_t i = 0; i < vector.dimension; i++) {
            stream >> vector.Data()[i];
        }
        return stream;
    }
    file::fileReaderWriter& operator>>(file::fileReaderWriter& stream,  IVector& vector) {
        stream << vector.dimension; 
        for (size_t i = 0; i < vector.dimension; i++) {
            stream << vector.Data()[i];
        }
        return stream;
    }
    bool IVector::operator!=(const IVector& other) {
            return !(*this == other);
    }
}
