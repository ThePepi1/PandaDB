#pragma once
#include "IVector.h"


class Vector_CPU : public IVector {
public:
    Vector_CPU(size_t dimension);
    Vector_CPU(size_t dimension, double* external_data);
    Vector_CPU(size_t dimension, double value);
    ~Vector_CPU() override = default;

    std::unique_ptr<IVector> operator+(IVector& other) override;
    std::unique_ptr<IVector> operator-(IVector& other) override;
    double operator*(IVector& other) override;
    std::unique_ptr<IVector> operator*(double scalar) override;
    std::unique_ptr<IVector> operator+(double scalar) override;
    std::unique_ptr<IVector> operator^(IVector& other) override;
    double norm() const override;
    std::unique_ptr<IVector> normalize() const override;
};

