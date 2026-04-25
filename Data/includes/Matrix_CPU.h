#pragma once
#include "Matrix.h"

namespace data {

    class Matrix_CPU : public IMatrix {
    public:
        Matrix_CPU(size_t rows, size_t cols);
        Matrix_CPU(size_t rows, size_t cols, double value);
        ~Matrix_CPU() override = default;

        std::unique_ptr<IMatrix> transpose() override;
        std::unique_ptr<IMatrix> operator+(IMatrix& other) override;
        std::unique_ptr<IMatrix> operator-(IMatrix& other) override;
        std::unique_ptr<IMatrix> operator*(IMatrix& other) override;
        std::unique_ptr<IMatrix> operator*(double scalar) override;
        std::unique_ptr<IMatrix> operator+(double scalar) override;
    };

}