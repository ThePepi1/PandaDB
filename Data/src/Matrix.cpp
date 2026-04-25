#include "../includes/Matrix.h"
#include <iostream>

namespace data {

    IMatrix::IMatrix(std::size_t r, std::size_t c)
        : rows(r), columns(c)
    {
        data = new double[rows * columns];
    }

    IMatrix::~IMatrix() {
        delete[] data;
    }
    IMatrix::IMatrix(std::size_t r, std::size_t c, double* external_data)
        : rows(r), columns(c), data(external_data) {}


    double& IMatrix::operator()(std::size_t row, std::size_t column) {
        if (row >= rows || column >= columns)
            throw std::out_of_range("Index out of bounds");

        return data[row * columns + column];
    }

    double* IMatrix::Data() { return data; }
    size_t IMatrix::Rows() { return rows; }
    size_t IMatrix::Columns() { return columns; }

    std::unique_ptr<IMatrix> IMatrix::operator~() {
        return this->transpose();
    }

    std::ostream& operator<<(std::ostream& stream, IMatrix& matrix) {
        for (size_t i = 0; i < matrix.rows; i++) {
            for (size_t j = 0; j < matrix.columns; j++) {
                stream << matrix(i, j) << " ";
            }
            stream << "\n";
        }
        return stream;
    }

    std::istream& operator>>(std::istream& stream, IMatrix& matrix) {
        for (size_t i = 0; i < matrix.rows; i++) {
            for (size_t j = 0; j < matrix.columns; j++) {
                stream >> matrix(i, j);
            }
        }
        return stream;
    }

}