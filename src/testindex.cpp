#include "../include/index/HnswVectorIndex.h"
#include "../include/index/PlainVectorIndex.h"
#include <chrono>
#include <random>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace vectordb;
using namespace std;
using namespace std::chrono;

// Configuration
const size_t DIM = 1000;                   // Vector dimension
const DistanceMetric METRIC = DistanceMetric::L2; // L2 or IP
const vector<size_t> TEST_SIZES = {10, 100, 1000, 10000, 50000, 100000};
const int TOP_K = 10;                   // Number of neighbors to search
const int SEARCH_QUERIES = 100;         // Number of test queries
const string OUTPUT_FILE = "benchmark_results.csv";

// Generate random vectors
vector<DenseVector> generateRandomVectors(size_t count, size_t dim) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<float> dist(-1.0, 1.0);

    vector<DenseVector> vectors;
    vectors.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        DenseVector vec(dim);
        for (size_t j = 0; j < dim; ++j) {
            vec[j] = dist(gen);
        }
        vectors.push_back(vec);
    }
    return vectors;
}

// Benchmark a single test size
void benchmarkSize(size_t vector_count, ofstream& outfile) {
    cout << "\nBenchmarking with " << vector_count << " vectors...\n";
    
    // Generate test data
    auto vectors = generateRandomVectors(vector_count, DIM);
    auto queries = generateRandomVectors(SEARCH_QUERIES, DIM);
    vector<pair<size_t, DenseVector>> points;
    for (size_t i = 0; i < vector_count; ++i) {
        points.emplace_back(i, vectors[i]);
    }

    // HNSW Benchmark
    cout << "Testing HNSW...\n";
    auto hnsw_start = high_resolution_clock::now();
    HnswVectorIndex hnsw_index(DIM, METRIC, vector_count * 1.1, 16, 200);
    hnsw_index.addBatch(points);
    auto hnsw_add_end = high_resolution_clock::now();
    
    vector<milliseconds> hnsw_search_times;
    for (const auto& query : queries) {
        auto start = high_resolution_clock::now();
        auto results = hnsw_index.search(query, TOP_K);
        auto end = high_resolution_clock::now();
        hnsw_search_times.push_back(duration_cast<milliseconds>(end - start));
    }
    auto hnsw_search_end = high_resolution_clock::now();

    // Plain Index Benchmark
    cout << "Testing Plain Index...\n";
    auto plain_start = high_resolution_clock::now();
    PlainVectorIndex plain_index(DIM, METRIC);
    plain_index.addBatch(points);
    auto plain_add_end = high_resolution_clock::now();
    
    vector<milliseconds> plain_search_times;
    for (const auto& query : queries) {
        auto start = high_resolution_clock::now();
        auto results = plain_index.search(query, TOP_K);
        auto end = high_resolution_clock::now();
        plain_search_times.push_back(duration_cast<milliseconds>(end - start));
    }
    auto plain_search_end = high_resolution_clock::now();

    // Calculate statistics
    auto hnsw_add_time = duration_cast<milliseconds>(hnsw_add_end - hnsw_start).count();
    auto plain_add_time = duration_cast<milliseconds>(plain_add_end - plain_start).count();
    
    auto avg_hnsw_search = accumulate(hnsw_search_times.begin(), hnsw_search_times.end(), milliseconds(0)).count() / SEARCH_QUERIES;
    auto avg_plain_search = accumulate(plain_search_times.begin(), plain_search_times.end(), milliseconds(0)).count() / SEARCH_QUERIES;

    // Output results
    outfile << vector_count << ","
            << hnsw_add_time << "," << avg_hnsw_search << ","
            << plain_add_time << "," << avg_plain_search << "\n";

    cout << fixed << setprecision(2);
    cout << "HNSW Results:\n";
    cout << "  - Insert Time: " << hnsw_add_time << " ms\n";
    cout << "  - Avg Search Time: " << avg_hnsw_search << " ms\n";
    cout << "Plain Results:\n";
    cout << "  - Insert Time: " << plain_add_time << " ms\n";
    cout << "  - Avg Search Time: " << avg_plain_search << " ms\n";
}

int main() {
    cout << "Starting benchmark comparison...\n";
    cout << "Vector Dimension: " << DIM << "\n";
    cout << "Distance Metric: " << (METRIC == DistanceMetric::L2 ? "L2" : "Inner Product") << "\n";
    
    ofstream outfile(OUTPUT_FILE);
    if (!outfile.is_open()) {
        cerr << "Error opening output file!\n";
        return 1;
    }

    // CSV header
    outfile << "Vector Count,HNSW Insert (ms),HNSW Search (ms),"
            << "Plain Insert (ms),Plain Search (ms)\n";

    // Run benchmarks for all test sizes
    for (size_t count : TEST_SIZES) {
        benchmarkSize(count, outfile);
    }

    outfile.close();
    cout << "\nBenchmark complete! Results saved to " << OUTPUT_FILE << endl;
    return 0;
}