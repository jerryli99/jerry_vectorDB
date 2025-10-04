#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <immintrin.h>
#include <algorithm>

class Timer {
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}
    double elapsed() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

// Portable SIMD detection
void check_simd_support() {
    std::cout << "SIMD Support Check:\n";
    
    // Check AVX
#ifdef __AVX__
    std::cout << "AVX: Supported\n";
#else
    std::cout << "AVX: Not supported\n";
#endif

    // Check AVX2  
#ifdef __AVX2__
    std::cout << "AVX2: Supported\n";
#else
    std::cout << "AVX2: Not supported\n";
#endif

    // Check FMA
#ifdef __FMA__
    std::cout << "FMA: Supported\n";
#else
    std::cout << "FMA: Not supported\n";
#endif
    std::cout << std::endl;
}

float horizontal_sum_avx(__m256 v) {
    __m128 vlow = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    
    __m128 shuf = _mm_shuffle_ps(vlow, vlow, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(vlow, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    
    return _mm_cvtss_f32(sums);
}

// Generate test data with specific dimension
void generateTestData(std::vector<float>& vectors, std::vector<float>& queries, 
                     int numVectors, int dim) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.1f, 1.0f); // Avoid very small numbers
    
    vectors.resize(numVectors * dim);
    queries.resize(dim);
    
    for (auto& val : vectors) val = dist(gen);
    for (auto& val : queries) val = dist(gen);
}

// 1. Naive scalar implementation with double accumulation for better accuracy
float l2_distance_naive(const float* a, const float* b, int dim) {
    double sum = 0.0; // Use double for accumulation to minimize error
    for (int i = 0; i < dim; i++) {
        double diff = (double)a[i] - (double)b[i];
        sum += diff * diff;
    }
    return (float)sum;
}

void knn_naive(const std::vector<float>& vectors, const std::vector<float>& query,
               std::vector<float>& distances, int numVectors, int dim) {
    for (int i = 0; i < numVectors; i++) {
        distances[i] = l2_distance_naive(&vectors[i * dim], query.data(), dim);
    }
}

// 2. SIMD with scalar cleanup - use double for scalar part to match naive accuracy
float l2_distance_avx_cleanup(const float* a, const float* b, int dim) {
    __m256 sum = _mm256_setzero_ps();
    int i = 0;
    
    // Process 8 elements at a time
    for (; i <= dim - 8; i += 8) {
        __m256 vecA = _mm256_loadu_ps(a + i);
        __m256 vecB = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(vecA, vecB);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    
    // Use double for scalar cleanup to match naive accuracy
    double result = (double)horizontal_sum_avx(sum);
    
    for (; i < dim; i++) {
        double diff = (double)a[i] - (double)b[i];
        result += diff * diff;
    }
    
    return (float)result;
}

void knn_avx_cleanup(const std::vector<float>& vectors, const std::vector<float>& query,
                     std::vector<float>& distances, int numVectors, int dim) {
    for (int i = 0; i < numVectors; i++) {
        distances[i] = l2_distance_avx_cleanup(&vectors[i * dim], query.data(), dim);
    }
}

// 3. SIMD with masked loads - also use double accumulation
float l2_distance_avx_masked(const float* a, const float* b, int dim) {
    __m256 sum = _mm256_setzero_ps();
    int i = 0;
    
    // Process full vectors
    for (; i <= dim - 8; i += 8) {
        __m256 vecA = _mm256_loadu_ps(a + i);
        __m256 vecB = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(vecA, vecB);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    
    // Handle remainder with masked load
    int remaining = dim - i;
    if (remaining > 0) {
        // Create mask for remaining elements
        __m256i mask = _mm256_setr_epi32(
            remaining > 0 ? -1 : 0,
            remaining > 1 ? -1 : 0,
            remaining > 2 ? -1 : 0,
            remaining > 3 ? -1 : 0,
            remaining > 4 ? -1 : 0,
            remaining > 5 ? -1 : 0,
            remaining > 6 ? -1 : 0,
            remaining > 7 ? -1 : 0
        );
        
        __m256 vecA = _mm256_maskload_ps(a + i, mask);
        __m256 vecB = _mm256_maskload_ps(b + i, mask);
        __m256 diff = _mm256_sub_ps(vecA, vecB);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    
    return (float)horizontal_sum_avx(sum); // Keep as float to see SIMD vs double difference
}

void knn_avx_masked(const std::vector<float>& vectors, const std::vector<float>& query,
                    std::vector<float>& distances, int numVectors, int dim) {
    for (int i = 0; i < numVectors; i++) {
        distances[i] = l2_distance_avx_masked(&vectors[i * dim], query.data(), dim);
    }
}

// 4. SIMD with loop unrolling and double cleanup
float l2_distance_avx_unrolled(const float* a, const float* b, int dim) {
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();
    __m256 sum4 = _mm256_setzero_ps();
    
    int i = 0;
    // Process 32 elements per iteration (4x unroll)
    for (; i <= dim - 32; i += 32) {
        __m256 a1 = _mm256_loadu_ps(a + i);
        __m256 b1 = _mm256_loadu_ps(b + i);
        __m256 diff1 = _mm256_sub_ps(a1, b1);
        sum1 = _mm256_fmadd_ps(diff1, diff1, sum1);
        
        __m256 a2 = _mm256_loadu_ps(a + i + 8);
        __m256 b2 = _mm256_loadu_ps(b + i + 8);
        __m256 diff2 = _mm256_sub_ps(a2, b2);
        sum2 = _mm256_fmadd_ps(diff2, diff2, sum2);
        
        __m256 a3 = _mm256_loadu_ps(a + i + 16);
        __m256 b3 = _mm256_loadu_ps(b + i + 16);
        __m256 diff3 = _mm256_sub_ps(a3, b3);
        sum3 = _mm256_fmadd_ps(diff3, diff3, sum3);
        
        __m256 a4 = _mm256_loadu_ps(a + i + 24);
        __m256 b4 = _mm256_loadu_ps(b + i + 24);
        __m256 diff4 = _mm256_sub_ps(a4, b4);
        sum4 = _mm256_fmadd_ps(diff4, diff4, sum4);
    }
    
    // Handle remaining 8-element chunks
    for (; i <= dim - 8; i += 8) {
        __m256 vecA = _mm256_loadu_ps(a + i);
        __m256 vecB = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(vecA, vecB);
        sum1 = _mm256_fmadd_ps(diff, diff, sum1);
    }
    
    // Combine all sums
    __m256 sum = _mm256_add_ps(_mm256_add_ps(sum1, sum2), _mm256_add_ps(sum3, sum4));
    double result = (double)horizontal_sum_avx(sum);
    
    // Scalar cleanup for final remainder with double
    for (; i < dim; i++) {
        double diff = (double)a[i] - (double)b[i];
        result += diff * diff;
    }
    
    return (float)result;
}

void knn_avx_unrolled(const std::vector<float>& vectors, const std::vector<float>& query,
                      std::vector<float>& distances, int numVectors, int dim) {
    for (int i = 0; i < numVectors; i++) {
        distances[i] = l2_distance_avx_unrolled(&vectors[i * dim], query.data(), dim);
    }
}

// More tolerant verification that understands floating-point differences
bool verify_results(const std::vector<float>& ref, const std::vector<float>& test, 
                   float relative_tolerance = 1e-4f, float absolute_tolerance = 1e-4f) {
    int mismatches = 0;
    const int max_mismatches = 5;
    
    for (size_t i = 0; i < ref.size(); i++) {
        float diff = std::abs(ref[i] - test[i]);
        float max_val = std::max(std::abs(ref[i]), std::abs(test[i]));
        
        // Check both relative and absolute tolerance
        if (diff > absolute_tolerance && diff > relative_tolerance * max_val) {
            if (mismatches < max_mismatches) {
                std::cout << "  Mismatch at index " << i << ": " << ref[i] << " vs " << test[i] 
                          << " (diff: " << diff << ", relative: " << (diff/max_val) << ")\n";
            }
            mismatches++;
            if (mismatches >= max_mismatches) break;
        }
    }
    
    if (mismatches > 0) {
        std::cout << "  " << mismatches << " mismatches found (tolerance: " 
                  << relative_tolerance << " relative, " << absolute_tolerance << " absolute)\n";
        return false;
    }
    return true;
}

void run_benchmark(int numVectors, int dim) {
    std::vector<float> vectors, query;
    generateTestData(vectors, query, numVectors, dim);
    
    std::vector<float> distances_naive(numVectors);
    std::vector<float> distances_avx_cleanup(numVectors);
    std::vector<float> distances_avx_masked(numVectors);
    std::vector<float> distances_avx_unrolled(numVectors);
    
    std::cout << "=== Benchmark: " << numVectors << " vectors × " << dim << " dimensions ===\n";
    std::cout << "Remainder elements: " << dim % 8 << "\n\n";
    
    // Warm-up run (optional, helps with consistent timing)
    knn_naive(vectors, query, distances_naive, numVectors, dim);
    
    // Test naive version
    Timer timer;
    knn_naive(vectors, query, distances_naive, numVectors, dim);
    double time_naive = timer.elapsed();
    
    // Test AVX cleanup version
    timer = Timer();
    knn_avx_cleanup(vectors, query, distances_avx_cleanup, numVectors, dim);
    double time_avx_cleanup = timer.elapsed();
    
    // Test AVX masked version
    timer = Timer();
    knn_avx_masked(vectors, query, distances_avx_masked, numVectors, dim);
    double time_avx_masked = timer.elapsed();
    
    // Test AVX unrolled version
    timer = Timer();
    knn_avx_unrolled(vectors, query, distances_avx_unrolled, numVectors, dim);
    double time_avx_unrolled = timer.elapsed();
    
    // Verify results with more reasonable tolerance
    std::cout << "Verification (tolerance: 0.1% relative or 0.001 absolute):\n";
    std::cout << "AVX Cleanup vs Naive: " << (verify_results(distances_naive, distances_avx_cleanup, 0.001f, 0.001f) ? "PASS" : "FAIL") << "\n";
    std::cout << "AVX Masked vs Naive: " << (verify_results(distances_naive, distances_avx_masked, 0.001f, 0.001f) ? "PASS" : "FAIL") << "\n";
    std::cout << "AVX Unrolled vs Naive: " << (verify_results(distances_naive, distances_avx_unrolled, 0.001f, 0.001f) ? "PASS" : "FAIL") << "\n\n";
    
    // Print results
    std::cout << "Performance Results:\n";
    printf("Naive scalar:        %.3f ms\n", time_naive * 1000);
    printf("AVX + cleanup:       %.3f ms (speedup: %.2fx)\n", time_avx_cleanup * 1000, time_naive/time_avx_cleanup);
    printf("AVX + masked:        %.3f ms (speedup: %.2fx)\n", time_avx_masked * 1000, time_naive/time_avx_masked);
    printf("AVX + unrolled:      %.3f ms (speedup: %.2fx)\n", time_avx_unrolled * 1000, time_naive/time_avx_unrolled);
    
    // Performance metrics
    double total_ops = numVectors * dim * 2.0; // 2 ops per element (sub + mul)
    std::cout << "\nPerformance Metrics (GFlops):\n";
    printf("Naive:    %.2f GFlops\n", total_ops / time_naive / 1e9);
    printf("Best AVX: %.2f GFlops\n", total_ops / std::min({time_avx_cleanup, time_avx_masked, time_avx_unrolled}) / 1e9);
    
    // Show which method was fastest
    double best_avx_time = std::min({time_avx_cleanup, time_avx_masked, time_avx_unrolled});
    std::string best_method;
    if (best_avx_time == time_avx_cleanup) best_method = "AVX + cleanup";
    else if (best_avx_time == time_avx_masked) best_method = "AVX + masked";
    else best_method = "AVX + unrolled";
    
    std::cout << "Fastest method: " << best_method << " (" << time_naive/best_avx_time << "x speedup)\n";
}

int main() {
    check_simd_support();
    
    // Test different dimensions to see remainder handling
    std::vector<std::pair<int, int>> test_cases = {
        {10000, 784},  // Perfect case: 784 % 8 = 0
        {10000, 787},  // Remainder 3
        {10000, 123},  // Remainder 3  
        {10000, 257},  // Remainder 1
        {10000, 1000}  // Remainder 4
    };
    
    for (const auto& test_case : test_cases) {
        run_benchmark(test_case.first, test_case.second);
        std::cout << "\n" << std::string(60, '=') << "\n\n";
    }
    
    return 0;
}