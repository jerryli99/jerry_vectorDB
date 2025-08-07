#include <faiss/IndexHNSW.h>
#include <chrono>
#include <iostream>
#include <random>
#include <malloc.h>     // For aligned_alloc
#include <sys/mman.h>   // For huge pages

// --- Memory Allocation Functions ---
void* alloc_aligned(size_t size, size_t alignment) {
    void* ptr = aligned_alloc(alignment, size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* alloc_huge(size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (ptr == MAP_FAILED) throw std::bad_alloc();
    return ptr;
}

// --- Benchmark Function ---
void benchmark(bool use_aligned, bool use_hugepages) {
    const size_t dim = 128;
    const size_t num_vectors = 10000; //
    const size_t total_bytes = num_vectors * dim * sizeof(float);

    // Allocate memory
    float* data;
    if (use_hugepages) {
        data = static_cast<float*>(alloc_huge(total_bytes));
        std::cout << "Using 2MB huge pages\n";
    } else if (use_aligned) {
        data = static_cast<float*>(alloc_aligned(total_bytes, 2 * 1024 * 1024));
        std::cout << "Using 2MB-aligned memory\n";
    } else {
        data = new float[num_vectors * dim];
        std::cout << "Using unaligned memory\n";
    }

    // Fill with random data
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (size_t i = 0; i < num_vectors * dim; ++i) {
        data[i] = dist(rng);
    }

    // Build HNSW index
    faiss::IndexHNSWFlat index(dim, 32);  // 32 is HNSW's M parameter
    auto start = std::chrono::high_resolution_clock::now();
    
    index.add(num_vectors, data);  // Build index
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Build time: " << duration.count() << " ms\n";
    std::cout << "Index size: " << index.ntotal << " vectors\n\n";

    // Cleanup
    if (use_hugepages) {
        munmap(data, total_bytes);
    } else if (use_aligned) {
        free(data);
    } else {
        delete[] data;
    }
}

// --- Main ---
int main() {
    std::cout << "=== Memory Alignment Benchmark ===\n";
    
    // Warm-up (Faiss has some one-time initialization overhead)
    std::cout << "Warming up...\n";
    benchmark(false, false);
    
    // Actual tests
    std::cout << "\n=== Benchmark Results ===\n";
    
    std::cout << "\n[1] Unaligned memory:\n";
    benchmark(false, false);
    
    std::cout << "\n[2] 2MB-aligned memory:\n";
    benchmark(true, false);
    
    std::cout << "\n[3] 2MB huge pages:\n";
    benchmark(false, true);
    
    return 0;
}