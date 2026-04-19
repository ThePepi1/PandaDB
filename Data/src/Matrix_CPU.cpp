#include "../includes/Matrix_CPU.h"

namespace data {
    Matrix_CPU::Matrix_CPU(std::size_t rows, std::size_t columns) : IMatrix(rows, columns) {
    }

    Matrix_CPU::Matrix_CPU(size_t rows, size_t cols, double value) : IMatrix(rows, cols) {
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*this)(i, j) = value;
            }
        }
    }

    IMatrix* Matrix_CPU::transpose() {
        Matrix_CPU * result = new Matrix_CPU (columns, rows); //n^2 very bad make it faster
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*result)(j, i) = (*this)(i, j);
            }
        }
        return result;
    }

    IMatrix* Matrix_CPU::operator+(const double scalar) {
        Matrix_CPU * result =new  Matrix_CPU(rows, columns);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*result)(i, j) = (*this)(i, j) + scalar;
            }
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator*(const double scalar) {
        Matrix_CPU * result = new Matrix_CPU (rows, columns); //n^2 very bad make it faster
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*result)(i, j) = (*this)(i, j) * scalar;
            }
        }
        return result;
    }

    IMatrix* Matrix_CPU::operator-(IMatrix &other) {
        Matrix_CPU * result = new Matrix_CPU (rows, columns); //n^2 very bad make it faster
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*result)(i, j) = (*this)(i, j) - other(i, j);
            }
        }
        return result;
    }

    IMatrix* Matrix_CPU::operator*(IMatrix &other) {
        Matrix_CPU * result = new Matrix_CPU (rows, other.Columns()); //n^2 very bad make it faster
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < other.Columns(); j++) {
                double sum = 0;
                for (size_t k = 0; k < columns; k++) {
                    sum += (*this)(i, k) * other(k, j);
                }
                (*result)(i, j) = sum;
            }
        }
        return result;
    }

    IMatrix* Matrix_CPU::operator+(IMatrix &other) {
        Matrix_CPU * result = new Matrix_CPU (rows, columns); //n^2 very bad make it faster
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                (*result)(i, j) = (*this)(i, j) + other(i, j);
            }
        }
        return result;
    }
}
