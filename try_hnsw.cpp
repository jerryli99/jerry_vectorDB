
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

#include <faiss/IndexHNSW.h>
#include <faiss/IndexFlat.h>

using idx_t = faiss::idx_t;
using namespace std::chrono;
using Clock = std::chrono::steady_clock;


// Mutex for thread-safe printing
std::mutex print_mutex;

void build_hnsw_batch(int batch_num, int d, int batch_size, float* xb_batch) {
    auto batch_start = Clock::now();
    
    faiss::IndexHNSWFlat hnsw_index(d, 64);
    hnsw_index.add(batch_size, xb_batch);
    
    auto batch_end = Clock::now();
    auto batch_time = duration_cast<milliseconds>(batch_end - batch_start).count();
    
    // Thread-safe printing
    std::lock_guard<std::mutex> lock(print_mutex);
    printf("Batch %d completed in %ld ms\n", batch_num, batch_time);
}

int main() {
    const int d = 1536;           // vector dimension
    const int nb_total = 1000000; // total vectors
    const int batch_size = 500;  // per batch
    const int nq = 10;           // number of queries
    const int k = 4;             // nearest neighbors
    const int num_batches = nb_total / batch_size;
    
    // Determine number of threads to use
    const unsigned int num_threads = std::thread::hardware_concurrency();
    printf("Using %u threads for parallel HNSW builds\n", num_threads);

    std::mt19937 rng;
    std::uniform_real_distribution<> distrib;

    // Allocate base and query vectors
    float* xb = new float[d * nb_total];
    float* xq = new float[d * nq];

    // Generate all 100,000 base vectors
    for (int i = 0; i < nb_total; i++) {
        for (int j = 0; j < d; j++) {
            xb[d * i + j] = distrib(rng);
        }
        xb[d * i] += i / 1000.0f;
    }

    // Generate query vectors
    for (int i = 0; i < nq; i++) {
        for (int j = 0; j < d; j++) {
            xq[d * i + j] = distrib(rng);
        }
        xq[d * i] += i / 1000.0f;
    }

    // --- Benchmark: multi-threaded small HNSW builds ---
    printf("Benchmarking parallel HNSW index builds...\n");
    auto start_small = Clock::now();

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    // Process batches in parallel
    for (int batch = 0; batch < num_batches; batch += num_threads) {
        // Launch threads for this batch group
        for (unsigned int t = 0; t < num_threads && (batch + t) < num_batches; t++) {
            int current_batch = batch + t;
            int offset = current_batch * batch_size;
            float* xb_batch = xb + d * offset;
            
            threads.emplace_back(build_hnsw_batch, current_batch, d, batch_size, xb_batch);
        }
        
        // Wait for all threads in this group to finish
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        threads.clear();
    }

    auto end_small = Clock::now();
    auto time_small = duration_cast<milliseconds>(end_small - start_small).count();
    printf("\nTotal time for parallel HNSW builds: %ld ms\n\n", time_small);

    // --- Clean up ---
    delete[] xb;
    delete[] xq;

    return 0;
}



// #include <cassert>
// #include <cstdio>
// #include <cstdlib>
// #include <random>
// #include <memory>
// #include <chrono>

// #include <faiss/IndexHNSW.h>
// #include <faiss/IndexFlat.h>

// using idx_t = faiss::idx_t;
// using namespace std::chrono;

// int main() {
//     const int d = 512;           // vector dimension
//     const int nb_total = 100000;  // total vectors
//     const int batch_size = 100;  // per batch
//     const int nq = 10;            // number of queries
//     const int k = 4;              // nearest neighbors
//     const int num_batches = nb_total / batch_size;

//     std::mt19937 rng;
//     std::uniform_real_distribution<> distrib;

//     // Allocate base and query vectors
//     float* xb = new float[d * nb_total];
//     float* xq = new float[d * nq];

//     // Generate all 100,000 base vectors
//     for (int i = 0; i < nb_total; i++) {
//         for (int j = 0; j < d; j++) {
//             xb[d * i + j] = distrib(rng);
//         }
//         xb[d * i] += i / 1000.0f;
//     }

//     // Generate query vectors
//     for (int i = 0; i < nq; i++) {
//         for (int j = 0; j < d; j++) {
//             xq[d * i + j] = distrib(rng);
//         }
//         xq[d * i] += i / 1000.0f;
//     }

//     // --- Benchmark: small HNSW builds ---
//     printf("Benchmarking independent HNSW index builds...\n");
//     auto start_small = Clock::now();

//     for (int batch = 0; batch < num_batches; ++batch) {
//         int offset = batch * batch_size;
//         float* xb_batch = xb + d * offset;

//         faiss::IndexHNSWFlat hnsw_index(d, 32);
//         hnsw_index.add(batch_size, xb_batch);
//     }

//     auto end_small = Clock::now();
//     auto time_small = duration_cast<milliseconds>(end_small - start_small).count();
//     printf("Total time for small HNSW builds: %ld ms\n\n", time_small);


//     // --- Benchmark: 1 large HNSW build ---
//     // printf("Benchmarking 1 large HNSW index build (100k vectors)...\n");
//     // auto start_big = Clock::now();

//     // faiss::IndexHNSWFlat hnsw_index_big(d, 32);
//     // hnsw_index_big.add(nb_total, xb);

//     // auto end_big = Clock::now();
//     // auto time_big = duration_cast<milliseconds>(end_big - start_big).count();
//     // printf("Total time for 1 large HNSW build (100k): %lld ms\n", time_big);


//     // --- Clean up ---
//     delete[] xb;
//     delete[] xq;

//     return 0;
// }
