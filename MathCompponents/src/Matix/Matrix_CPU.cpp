#include "../../includes/Matrix/Matrix_CPU.h"
#include <array>
#include <immintrin.h>
#include <stdexcept>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include "../../../Utils/config/CASH_SIZE/CASH_SIZE.h"

namespace data {
    static void multiply_block(double *output, double *left, double *right, size_t N, size_t K) {
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        constexpr int output_reg_count = {config::ROWS_PER_REGISTER * config::COLS_PER_REGISTER / avx_double};
        std::array<__m256d, output_reg_count> result;
        for (int i = 0; i < output_reg_count; ++i) {
            result[i] = _mm256_setzero_pd();
        }
        for (int k = 0; k < config::SHARED_PER_CACHE_BLOCK; ++k, right += N) {
            double *left_cache = left;
            int idx = 0;
            for (int i = 0; i < config::ROWS_PER_REGISTER; ++i, left_cache += K) {
                __m256d left_reg = _mm256_broadcast_sd(left_cache + k);
                for (int j = 0; j < config::COLS_PER_REGISTER; j += avx_double, ++idx) {
                    __m256d right_reg = _mm256_loadu_pd(right + j);
                    result[idx] = _mm256_fmadd_pd(left_reg, right_reg, result[idx]);
                }
            }
        }
        int idx = 0;
        for (int i = 0; i < config::ROWS_PER_REGISTER; ++i, output += N) {
            for (int j = 0; j < config::COLS_PER_REGISTER; j += 4, ++idx) {
                __m256d out = _mm256_loadu_pd(output + j);
                __m256d res = result[idx];
                out = _mm256_add_pd(out, res);
                _mm256_storeu_pd(output + j, out);
            }
        }
    }

    Matrix_CPU::Matrix_CPU(std::size_t rows, std::size_t columns) : IMatrix(rows, columns) {
    }

    Matrix_CPU::Matrix_CPU(size_t rows, size_t cols, double value) : IMatrix(rows, cols) {
        double *data = this->Data();
        size_t total = rows * cols;
        for (size_t i = 0; i < total; i++) {
            data[i] = value;
        }
    }

    std::unique_ptr<IMatrix> Matrix_CPU::transpose() {
        auto result = std::make_unique<Matrix_CPU>(columns, rows);
        double *data_me = this->Data();
        double *data_result = result->Data();
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                data_result[j * rows + i] = data_me[i * columns + j];
            }
        }
        return result;
    }

    std::unique_ptr<IMatrix> Matrix_CPU::operator+(const double scalar) {
        auto result = std::make_unique<Matrix_CPU>(rows, columns);
        double *data_me = this->Data();
        double *data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] + scalar;
        }
        return result;
    }

    std::unique_ptr<IMatrix> Matrix_CPU::operator+(IMatrix &other) {
        if (rows != other.Rows() || columns != other.Columns())
            throw std::invalid_argument("Matrix dimensions must match for addition");
        auto result = std::make_unique<Matrix_CPU>(rows, columns);
        double *data_me = this->Data();
        double *data_other = other.Data();
        double *data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] + data_other[i];
        }
        return result;
    }

    std::unique_ptr<IMatrix> Matrix_CPU::operator*(const double scalar) {
        auto result = std::make_unique<Matrix_CPU>(rows, columns);
        double *data_me = this->Data();
        double *data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] * scalar;
        }
        return result;
    }

    std::unique_ptr<IMatrix> Matrix_CPU::operator-(IMatrix &other) {
        if (rows != other.Rows() || columns != other.Columns())
            throw std::invalid_argument("Matrix dimensions must match for subtraction");
        auto result = std::make_unique<Matrix_CPU>(rows, columns);
        double *data_me = this->Data();
        double *data_other = other.Data();
        double *data_result = result->Data();
        size_t total = rows * columns;
        for (size_t i = 0; i < total; i++) {
            data_result[i] = data_me[i] - data_other[i];
        }
        return result;
    }

    std::unique_ptr<IMatrix> Matrix_CPU::operator*(IMatrix &other) {
        const size_t M = rows;
        const size_t K = columns;
        const size_t N = other.Columns();
        if (K != other.Rows())
            throw std::invalid_argument("Inner matrix dimensions must match for multiplication");

        auto result = std::make_unique<Matrix_CPU>(M, N , 0.0);

        double *left = this->Data();
        double *right = other.Data();
        double *output = result->Data();

        const size_t M_blocks = (M / config::ROWS_PER_L2_BLOCK) * config::ROWS_PER_L2_BLOCK;
        const size_t K_blocks = (K / config::SHARED_PER_CACHE_BLOCK) * config::SHARED_PER_CACHE_BLOCK;
        const size_t N_blocks = (N / config::COLS_PER_L1_BLOCK) * config::COLS_PER_L1_BLOCK;

        tbb::parallel_for(
            tbb::blocked_range2d<size_t>(0, M_blocks / config::ROWS_PER_L2_BLOCK,
                                         0, N_blocks / config::COLS_PER_L1_BLOCK),
            [&](const tbb::blocked_range2d<size_t> &r) {
                for (size_t ib = r.rows().begin(); ib != r.rows().end(); ++ib) {
                    for (size_t jb = r.cols().begin(); jb != r.cols().end(); ++jb) {
                        size_t i_base = ib * config::ROWS_PER_L2_BLOCK;
                        size_t j_base = jb * config::COLS_PER_L1_BLOCK;

                        for (size_t kb = 0; kb < K_blocks; kb += config::SHARED_PER_CACHE_BLOCK) {
                            double *left_cache = left + i_base * K + kb;
                            double *right_cache = right + kb * N + j_base;
                            double *output_cache = output + i_base * N + j_base;

                            for (size_t i2 = 0; i2 < config::ROWS_PER_L2_BLOCK; i2 += config::ROWS_PER_REGISTER)
                                for (size_t j2 = 0; j2 < config::COLS_PER_L1_BLOCK; j2 += config::COLS_PER_REGISTER) {
                                    double *left_reg = left_cache + i2 * K;
                                    double *right_reg = right_cache + j2;
                                    double *output_reg = output_cache + i2 * N + j2;

                                    multiply_block(output_reg, left_reg, right_reg, N, K);
                                }
                        }
                    }
                }
            }, tbb::auto_partitioner()
        );

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
