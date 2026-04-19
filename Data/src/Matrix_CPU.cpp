#include "../includes/Matrix_CPU.h"
namespace data {
    Matrix_CPU::Matrix_CPU(std::size_t rows, std::size_t columns) : IMatrix(rows, columns) {
    }
    Matrix_CPU::Matrix_CPU(size_t rows, size_t cols, double value) : IMatrix(rows, cols) {
        double* data = this->Data();
        size_t total = rows * cols;
        for (size_t i = 0; i < total; i++) {
            data[i] = value;
        }
    }
    IMatrix* Matrix_CPU::transpose() {
        Matrix_CPU* result = new Matrix_CPU(columns, rows);
        double* data_me = this->Data();
        double* data_result = result->Data();
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                data_result[j * rows + i] = data_me[i * columns + j];
            }
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator+(const double scalar) {
        Matrix_CPU* result = new Matrix_CPU(rows, columns);
        double* data_me = this->Data();
        double* data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] + scalar;
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator+(IMatrix& other) {
        Matrix_CPU* result = new Matrix_CPU(rows, columns);
        double* data_me = this->Data();
        double* data_other = other.Data();
        double* data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] + data_other[i];
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator*(const double scalar) {
        Matrix_CPU* result = new Matrix_CPU(rows, columns);
        double* data_me = this->Data();
        double* data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] * scalar;
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator-(IMatrix& other) {
        Matrix_CPU* result = new Matrix_CPU(rows, columns);
        double* data_me = this->Data();
        double* data_other = other.Data();
        double* data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] - data_other[i];
        }
        return result;
    }
    IMatrix* Matrix_CPU::operator*(IMatrix& other) {
        Matrix_CPU* result = new Matrix_CPU(rows, other.Columns(), 0);
        double* data_me = this->Data();
        double* data_other = other.Data();
        double* result_me = result->Data();
        size_t N = other.Columns();
        for (size_t i = 0; i < rows; ++i)
            for (size_t k = 0; k < columns; ++k) {
                double a_ik = data_me[i * columns + k];
                for (size_t j = 0; j < N; ++j)
                    result_me[i * N + j] += a_ik * data_other[k * N + j];
            }
        return result;
    }
}