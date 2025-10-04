#pragma once

#include "DataTypes.h"

#include <immintrin.h>
#include <cmath>

/**
 * @details
    L2 = sqrt((a₁-b₁)² + (a₂-b₂)² + ...)
    DOT = a₁*b₁ + a₂*b₂ + a₃*b₃ + ...
    COSINE = (a·b) / (|a| * |b|)

    _mm256_loadu_ps(): Load 8 floats from memory

    _mm256_fmadd_ps(): Multiply 8 pairs and add to accumulator (all at once!)

    _mm256_storeu_ps(): Store 8 results back

    Remainder handling: Process leftover elements (1-7) with regular loops
*/
namespace vectordb {
//SIMD + Scalar Helpers
#ifdef __AVX2__
inline float avx_dot(const float* a, const float* b, size_t dim) noexcept {
    size_t i = 0;
    __m256 vsum = _mm256_setzero_ps();
    // process 8 floating-point numbers in parallel using 256-bit registers
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        vsum = _mm256_fmadd_ps(va, vb, vsum); // vsum += va * vb
    }

    float tmp[8];
    _mm256_storeu_ps(tmp, vsum);
    float sum = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];

    // handle remainder
    for (; i < dim; ++i)
        sum += a[i] * b[i];

    return sum;
}

inline float avx_l2sq(const float* a, const float* b, size_t dim) noexcept {
    size_t i = 0;
    __m256 vsum = _mm256_setzero_ps();

    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        vsum = _mm256_fmadd_ps(diff, diff, vsum);
    }

    float tmp[8];
    _mm256_storeu_ps(tmp, vsum);
    float sum = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];

    for (; i < dim; ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }

    return sum;
}
#endif //__AVX2__


inline float scalar_dot(const float* a, const float* b, size_t dim) noexcept {
    float s = 0.0f;
    for (size_t i = 0; i < dim; ++i) s += a[i] * b[i];
    return s;
}

inline float scalar_l2sq(const float* a, const float* b, size_t dim) noexcept {
    float s = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

inline float norm(const float* a, size_t dim) noexcept {
#ifdef __AVX2__
    float dot = avx_dot(a, a, dim);
#else
    float dot = scalar_dot(a, a, dim);
#endif
    return std::sqrt(dot);
}


//main public API
inline float compute_distance(DistanceMetric metric,
                              const float* a,
                              const float* b,
                              size_t dim) {
    switch (metric) {
        case DistanceMetric::L2: {
#ifdef __AVX2__
            return avx_l2sq(a, b, dim);
#else
            return scalar_l2sq(a, b, dim);
#endif
        }

        case DistanceMetric::DOT: {
#ifdef __AVX2__
            return avx_dot(a, b, dim);
#else
            return scalar_dot(a, b, dim);
#endif
        }

        case DistanceMetric::COSINE: {
#ifdef __AVX2__
            float dot = avx_dot(a, b, dim);
#else
            float dot = scalar_dot(a, b, dim);
#endif
            float norm_a = norm(a, dim);
            float norm_b = norm(b, dim);
            if (norm_a < 1e-12f) norm_a = 1e-12f;
            if (norm_b < 1e-12f) norm_b = 1e-12f;
            return dot / (norm_a * norm_b);  // cosine similarity
        }

        default:
            throw std::runtime_error("Unknown DistanceMetric");
            //not sure if i still need this default if i already checked at the receiving api part.
            //whatever.
    }
}

// Overloads for DenseVector or std::vector<float>
template <typename VecType>
inline float compute_distance(DistanceMetric metric,
                              const VecType& a,
                              const VecType& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("Dimension mismatch in compute_distance()");
    }
    return compute_distance(metric, a.data(), b.data(), a.size());
}

} // namespace vectordb

