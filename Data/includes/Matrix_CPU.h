#pragma once
#include <cstddef>
#include <iosfwd>

#include "Matrix.h"


namespace data {

class Matrix_CPU : public IMatrix  {
    public:
    ~Matrix_CPU() override = default;
    Matrix_CPU(size_t rows, size_t cols);
    Matrix_CPU(size_t rows, size_t cols, double value);
    IMatrix* transpose() override;
    IMatrix* operator+( IMatrix & other) override;
    IMatrix* operator-( IMatrix & other) override;
    IMatrix* operator*( IMatrix & other) override;
    IMatrix* operator*(const double scalar) override; // No need for / it is 1/scalar
    IMatrix* operator+(const double scalar) override; // no need for - it is -scalar :D
};
}
