#include "../includes/Matrix.h"
#include <iostream>

namespace data {

    IMatrix::IMatrix(std::size_t r, std::size_t c)
        : rows(r), columns(c), capacity(c)
    {
        data = new double[rows * capacity];
    }

    IMatrix::~IMatrix() {
        delete[] data;
    }

    void IMatrix::ensure_capacity(std::size_t new_cols) {
        if (new_cols <= capacity) return;

        std::size_t new_capacity = capacity * 2;
        if (new_capacity < new_cols) new_capacity = new_cols;

        double* new_data = new double[rows * new_capacity];

        for (size_t i = 0; i < rows; i++)
            for (size_t j = 0; j < columns; j++)
                new_data[i * new_capacity + j] = data[i * capacity + j];

        delete[] data;
        data = new_data;
        capacity = new_capacity;
    }

    void IMatrix::push_column(const double* col_values) {
        ensure_capacity(columns + 1);

        for (size_t i = 0; i < rows; i++) {
            data[i * capacity + columns] = col_values[i];
        }
        columns++;
    }

    double& IMatrix::operator()(std::size_t row, std::size_t column) {
        if (row >= rows || column >= columns)
            throw std::out_of_range("Index out of bounds");

        return data[row * capacity + column];
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