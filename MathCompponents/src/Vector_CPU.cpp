
#include "../includes/Vector_CPU.h"

#include <cmath>
#include <stdexcept>


Vector_CPU::Vector_CPU(size_t dimension):IVector(dimension) {}
Vector_CPU::Vector_CPU(size_t dimension, double* external_data) :IVector(dimension, external_data) {}
Vector_CPU::Vector_CPU(size_t dimension, double value): IVector(dimension, value) {}

std::unique_ptr<IVector> Vector_CPU::operator+(double scalar) {
    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
    for (size_t i = 0; i < dimension; i++)
        (*result)(i) = (*this)(i) + scalar;
    return result;
}

std::unique_ptr<IVector> Vector_CPU::operator+(IVector &other) {
    if (dimension != other.Dimension())
        throw std::invalid_argument("Dimension mismatch");

    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
    for (size_t i = 0; i < dimension; i++)
        (*result)(i) = (*this)(i) + other(i);
    return result;
}

std::unique_ptr<IVector> Vector_CPU::operator-(IVector &other) {
    if (dimension != other.Dimension())
        throw std::invalid_argument("Dimension mismatch");

    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
    for (size_t i = 0; i < dimension; i++)
        (*result)(i) = (*this)(i) - other(i);
    return result;
}

std::unique_ptr<IVector> Vector_CPU::operator*(double scalar) {
    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
    for (size_t i = 0; i < dimension; i++)
        (*result)(i) = (*this)(i) * scalar;
    return result;
}

double Vector_CPU::operator*(IVector &other) {
    if (dimension != other.Dimension())
        throw std::invalid_argument("Dimension mismatch");

    double result = 0.0;
    for (size_t i = 0; i < dimension; i++)
        result += (*this)(i) * other(i);
    return result;
}

std::unique_ptr<IVector> Vector_CPU::operator^(IVector &other) {
    if (dimension != 3 || other.Dimension() != 3)
        throw std::invalid_argument("Cross product is only defined for 3D vectors");

    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(3);
    (*result)(0) = (*this)(1) * other(2) - (*this)(2) * other(1);
    (*result)(1) = (*this)(2) * other(0) - (*this)(0) * other(2);
    (*result)(2) = (*this)(0) * other(1) - (*this)(1) * other(0);
    return result;
}

double Vector_CPU::norm() const {
    double sum = 0.0;
    for (size_t i = 0; i < dimension; i++)
        sum += (*this)(i) * (*this)(i);
    return std::sqrt(sum);
}

std::unique_ptr<IVector> Vector_CPU::normalize() const {
    double n = norm();
    if (n < 1e-10)
        throw std::runtime_error("Cannot normalize zero vector");
    std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
    for (size_t i = 0; i < dimension; i++)
        (*result)(i) = (*this)(i) / n;
    return result;
}


