#pragma once
#include "IVector.h"
namespace data {
    class Vector_CPU : public IVector {
    public:
        Vector_CPU(size_t dimension);
        Vector_CPU(size_t dimension, double* external_data);
        Vector_CPU(size_t dimension, double value);
        ~Vector_CPU() override = default;

        std::unique_ptr<IVector> operator+(const IVector& other) const override;
        std::unique_ptr<IVector> operator-(const IVector& other) const override;
        double operator*(const IVector& other) const override;
        std::unique_ptr<IVector> operator/(double scalar) const override;
        std::unique_ptr<IVector> operator*(double scalar) const override;
        std::unique_ptr<IVector> operator+(double scalar) const override;
        std::unique_ptr<IVector> operator^(const IVector& other) const override;
        double norm() const override;
        std::unique_ptr<IVector> normalize() const override;
    };
}
