
#include "../../includes/Vector/Vector_CPU.h"
#include "Vector/IVector.h"

#include <immintrin.h>
#include <cmath>
#include <memory>
#include <stdexcept>
namespace data {
    Vector_CPU::Vector_CPU(size_t dimension):IVector(dimension) {}
    Vector_CPU::Vector_CPU(size_t dimension, double* external_data) :IVector(dimension, external_data) {}
    Vector_CPU::Vector_CPU(size_t dimension, double value): IVector(dimension, value) {}
    Vector_CPU::Vector_CPU(size_t dimension, double* external_data, bool owner): IVector(dimension, external_data, owner) {}
    Vector_CPU::Vector_CPU(): IVector() {}

    std::unique_ptr<IVector> Vector_CPU::operator+(double scalar) const{
        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
        double* resultDouble = result->Data();
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i+avx_double <= dimension; i += avx_double, resultDouble += avx_double, thisData += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_add_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData)
            *resultDouble = *thisData + scalar;
        return result;
    }

    std::unique_ptr<IVector> Vector_CPU::operator+(const IVector &other) const {
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");

        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
        double * resultDouble = result->Data();
        double *thisData = Data();
        double *otherData = other.Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        size_t i = 0;
        for (; i+ avx_double <= dimension; i+=avx_double, thisData += avx_double, resultDouble += avx_double, otherData += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_add_pd(_mm256_loadu_pd(thisData), _mm256_loadu_pd(otherData)));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData, ++otherData)
            *resultDouble = *thisData + *otherData;

        return  result;
    }

    std::unique_ptr<IVector> Vector_CPU::operator-(const IVector &other) const{
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");
        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
        double * resultDouble = result->Data();
        double *thisData = Data();
        double *otherData = other.Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+= avx_double, thisData += avx_double, resultDouble += avx_double, otherData += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_sub_pd(_mm256_loadu_pd(thisData), _mm256_loadu_pd(otherData)));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData, ++otherData)
            *resultDouble = *thisData - *otherData;

        return  result;

    }

    std::unique_ptr<IVector> Vector_CPU::operator*(double scalar) const {
        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
        double* resultDouble = result->Data();
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double, resultDouble += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_mul_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData)
            *resultDouble = *thisData * scalar;

        return result;
    }

    std::unique_ptr<IVector> Vector_CPU::operator/(double scalar)const {
        if (std::abs(scalar) < 1e-12)
            throw std::runtime_error("Division by zero");
        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(dimension);
        double* resultDouble = result->Data();
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double, resultDouble += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_div_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData)
            *resultDouble = *thisData / scalar;

        return result;
    }

    double Vector_CPU::operator*(const IVector &other) const{
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");

        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d sum = _mm256_setzero_pd();
        double* thisData = Data();
        double *otherData = other.Data();
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double, otherData += avx_double) {
            sum = _mm256_add_pd(sum, _mm256_mul_pd(_mm256_loadu_pd(thisData), _mm256_loadu_pd(otherData)));
        }
        __m128d lo     = _mm256_castpd256_pd128(sum);
        __m128d hi     = _mm256_extractf128_pd(sum,1);
        __m128d sum128 = _mm_add_pd(lo, hi);
        __m128d tmp    = _mm_shuffle_pd(sum128, sum128, 1);
        double  result = _mm_cvtsd_f64(_mm_add_pd(sum128, tmp));
        for (; i < dimension; ++i, ++thisData, ++otherData)
            result += (*thisData) * (*otherData);

        return result;
    }

    std::unique_ptr<IVector> Vector_CPU::operator^(const IVector &other) const {
        if (dimension != 3 || other.Dimension() != 3)
            throw std::invalid_argument("Cross product is only defined for 3D vectors");

        std::unique_ptr<IVector> result = std::make_unique<Vector_CPU>(3);
        (*result)(0) = (*this)(1) * other(2) - (*this)(2) * other(1);
        (*result)(1) = (*this)(2) * other(0) - (*this)(0) * other(2);
        (*result)(2) = (*this)(0) * other(1) - (*this)(1) * other(0);
        return result;
    }

    double Vector_CPU::norm() const {
        double result = (*this) * (*this);
        return std::sqrt(result);
    }

    std::unique_ptr<IVector> Vector_CPU::normalize() const{
        double n = norm();
        if (n < 1e-10)
            throw std::runtime_error("Cannot normalize zero vector");
        return (*this) / n;
    }
    IVector& Vector_CPU::operator+=(const IVector &other) {
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");
        double * resultDouble = Data();
        double *thisData = Data();
        double *otherData = other.Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        size_t i = 0;
        for (; i+ avx_double <= dimension; i+=avx_double, thisData += avx_double, resultDouble += avx_double, otherData += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_add_pd(_mm256_loadu_pd(thisData), _mm256_loadu_pd(otherData)));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData, ++otherData)
            *resultDouble = *thisData + *otherData;

        return *this;
    }
    IVector& Vector_CPU::operator-=(const IVector &other) {
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");
        double * resultDouble = Data();
        double *thisData = Data();
        double *otherData = other.Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+= avx_double, thisData += avx_double, resultDouble += avx_double, otherData += avx_double) {
            _mm256_storeu_pd(resultDouble, _mm256_sub_pd(_mm256_loadu_pd(thisData), _mm256_loadu_pd(otherData)));
        }
        for (; i < dimension; ++i, ++resultDouble, ++thisData, ++otherData)
            *resultDouble = *thisData - *otherData;

        return *this;
    }
    IVector& Vector_CPU::operator*=(double scalar) {
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double) {
            _mm256_storeu_pd(thisData, _mm256_mul_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++thisData)
            *thisData *= scalar;

        return *this;
    }
    IVector& Vector_CPU::operator/=(double scalar) {
        if (std::abs(scalar) < 1e-12)
            throw std::runtime_error("Division by zero");
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double) {
            _mm256_storeu_pd(thisData, _mm256_div_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++thisData)
            *thisData /= scalar;
        return *this;
    }
    IVector& Vector_CPU::operator+=(double scalar) {
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData += avx_double) {
            _mm256_storeu_pd(thisData, _mm256_add_pd(_mm256_loadu_pd(thisData), scalarRegister));
        }
        for (; i < dimension; ++i, ++thisData)
            *thisData += scalar;

        return *this;
    }
    IVector& Vector_CPU::operator-=(double scalar) {
        double* thisData = Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        __m256d scalarRegister = _mm256_set1_pd(scalar);
        size_t i = 0;
        for (; i + avx_double <= dimension; i+=avx_double, thisData
    += avx_double) {
                _mm256_storeu_pd(thisData, _mm256_sub_pd(_mm256_loadu_pd(thisData), scalarRegister));
            }
        for (; i < dimension; ++i, ++thisData)
            *thisData -= scalar;
        return *this;
    }
    bool Vector_CPU::operator==(const IVector &other) {
        if (dimension != other.Dimension())
            return false;
        double *thisData = Data();
        double *otherData = other.Data();
        for (size_t i = 0; i < dimension; ++i, ++thisData, ++otherData) {
            if (std::abs(*thisData - *otherData) > 1e-12)
                return false;
        }
        return true;
    }
    void Vector_CPU::operator=(const IVector &other) {
        if (dimension != other.Dimension())
            throw std::invalid_argument("Dimension mismatch");
        double *thisData = Data();
        double *otherData = other.Data();
        for (size_t i = 0; i < dimension; ++i, ++thisData, ++otherData) {
            *thisData = *otherData;
        }
    }
    std::unique_ptr<IVector> IVector::clone(size_t start, size_t dimension) const {
        return std::make_unique<Vector_CPU>(dimension, Data() + start);
    }

}

