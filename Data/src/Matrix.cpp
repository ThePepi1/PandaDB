#include "../includes/Matrix.h"

#include <fstream>

namespace data {
    IMatrix::IMatrix(std::size_t rows, std::size_t columns): rows(rows), columns(columns) {
        data = new double[rows * columns];
    }
    IMatrix::~IMatrix() = default;

    size_t IMatrix::Columns() {
        return columns;
    }
    size_t IMatrix::Rows() {
        return rows;
    }
    double & IMatrix::operator()(std::size_t row, std::size_t column) {
        if (row < 0 || row >= rows) {
            throw std::out_of_range("rows out of bound");
        }
        if (column < 0 || column >= columns) {
            throw std::out_of_range("colum out of bound");
        }
        return data[row * columns + column];
    }

    double *IMatrix::Data() {
        return data;
    }
    std::ostream &operator<<(std::ostream &stream, IMatrix &matrix) {
        for (size_t i = 0; i < matrix.rows; i++) {
            for (size_t j = 0; j < matrix.columns; j++) {
                stream << matrix(i, j);
            }
        }
        return stream;
    }
    std::istream &operator>>(std::istream &stream, IMatrix &matrix) {
        for (size_t i = 0; i < matrix.rows; i++) {
            for (size_t j = 0; j < matrix.columns; j++) {
                stream >> matrix(i, j);
            }
        }
        return stream;
    }

    IMatrix* IMatrix::operator~() {
        return  this->transpose();
    }
}
