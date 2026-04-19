
#include "../includes/Matrix_CPU.h"

#include <array>
#include <immintrin.h>

#include "../../Utils/config/CASH_SIZE/CASH_SIZE.h"
namespace data {
    static void multiply_block(double* output, const double* left, const double* right, size_t N, size_t K) {
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        for (int i = 0; i < config::ROWS_PER_REGISTER; ++i, output += N, left += K) {
            const double* right_cache = right ;
            for (int k = 0; k < config::SHARED_PER_CACHE_BLOCK; ++k, right_cache += N) {
                __m256d left_reg = _mm256_broadcast_sd(left + K);
                for (int j = 0; j < config::COLS_PER_REGISTER; j += avx_double) {
                    __m256d right_reg = _mm256_loadu_pd(right_cache + j);
                    __m256d output_reg = _mm256_loadu_pd(output + j);
                    output_reg = _mm256_fmadd_pd(left_reg, right_reg, output_reg);
                    _mm256_storeu_pd(output + j, output_reg);
                }
            }
        }
    }
    Matrix_CPU::Matrix_CPU(std::size_t rows, std::size_t columns) : IMatrix(rows, columns) {}
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
        const size_t M = rows;
        const size_t K = columns;
        const size_t N = other.Columns();

        Matrix_CPU* result = new Matrix_CPU(M, N, 0);
        double* left   = this->Data();
        double* right  = other.Data();
        double* output = result->Data();
        //Make lefovers out :D do i need it prbably not
        const size_t M_blocks = (M / config::ROWS_PER_L2_BLOCK)      * config::ROWS_PER_L2_BLOCK;
        const size_t K_blocks = (K / config::SHARED_PER_CACHE_BLOCK)  * config::SHARED_PER_CACHE_BLOCK;
        const size_t N_blocks = (N / config::COLS_PER_L1_BLOCK)       * config::COLS_PER_L1_BLOCK;


        for (size_t ib = 0; ib < M_blocks; ib += config::ROWS_PER_L2_BLOCK)
            for (size_t kb = 0; kb < K_blocks; kb += config::SHARED_PER_CACHE_BLOCK)
                for (size_t jb = 0; jb < N_blocks; jb += config::COLS_PER_L1_BLOCK) {
                    double* left_cache   = left   + ib * K + kb;
                    double* right_cache  = right  + kb * N + jb;
                    double* output_cache = output + ib * N + jb;

                    for (size_t i2 = 0; i2 < config::ROWS_PER_L2_BLOCK; i2 += config::ROWS_PER_REGISTER)
                        for (size_t j2 = 0; j2 < config::COLS_PER_L1_BLOCK; j2 += config::COLS_PER_REGISTER) {

                            double* left_reg   = left_cache   + i2 * K;
                            double* right_reg  = right_cache  + j2;
                            double* output_reg = output_cache + i2 * N + j2;

                            multiply_block(output_reg, left_reg, right_reg, N, K);
                        }
                }

        for (size_t i = 0; i < M; ++i)
            for (size_t k = 0; k < K; ++k) {
                double left_val = left[i * K + k];
                for (size_t j = N_blocks; j < N; ++j)
                    output[i * N + j] += left_val * right[k * N + j];
                if (i >= M_blocks || k >= K_blocks)
                    for (size_t j = 0; j < N_blocks; ++j)
                        output[i * N + j] += left_val * right[k * N + j];
            }

        return result;
    }
}