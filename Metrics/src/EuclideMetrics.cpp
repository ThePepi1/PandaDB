//
// Created by pepi on 5/8/26.
//

#include "../includes/EuclideMetrics.h"

#include <cmath>
#include <math.h>
#include <immintrin.h>


#include "../includes/MetricFactory.h"

namespace metrics {
    EuclideMetrics::EuclideMetrics() = default;

    std::string_view EuclideMetrics::name() const {
        return "euclide";
    }

    double EuclideMetrics::operator()(const data::IVector &first, const data::IVector &second) const {
        if (first.Dimension() != second.Dimension()) {
            throw std::invalid_argument("Vectors must be of the same size");
        }
        __m256d sum = _mm256_setzero_pd();
        double* firstData = first.Data();
        double* secondData = second.Data();
        constexpr int avx_double = 256 / (sizeof(double) * 8);
        size_t i = 0;
        for (; i + avx_double < first.Dimension(); i+= avx_double) {
            __m256d div = _mm256_sub_pd(_mm256_loadu_pd(firstData), _mm256_loadu_pd(secondData));
            sum = _mm256_add_pd(sum, _mm256_mul_pd(div, div));
            firstData += avx_double;
            secondData += avx_double;
        }
        __m128d lo     = _mm256_castpd256_pd128(sum);
        __m128d hi     = _mm256_extractf128_pd(sum,1);
        __m128d sum128 = _mm_add_pd(lo, hi);
        __m128d tmp    = _mm_shuffle_pd(sum128, sum128, 1);
        double  result = _mm_cvtsd_f64(_mm_add_pd(sum128, tmp));
        for (; i < first.Dimension(); ++i, ++firstData, ++secondData) {
            double diff = (*firstData) - (*secondData);
            result += diff * diff;
        }
        return sqrt(result);
    }

}