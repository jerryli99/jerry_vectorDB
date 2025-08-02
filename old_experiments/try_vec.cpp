#include <iostream>
#include <Eigen/Dense>
#include <faiss/IndexHNSW.h>
#include <faiss/Index.h>

using DenseVector = Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>;
using AppendableStorage = std::vector<DenseVector>;
using idx_t = faiss::idx_t;
int main() {
    const size_t dim = 3;
    const size_t num_vectors = 5;

    // Step 1: Create dummy append store
    AppendableStorage append_store;
    for (size_t i = 0; i < num_vectors; ++i) {
        DenseVector vec(1, dim);
        vec << i, i + 1, i + 2;

        std::cout << "Before move: vec " << i
                  << " shape = [" << vec.rows() << " x " << vec.cols() << "], values = " << vec << "\n";

        append_store.push_back(std::move(vec));
    }

    // Step 2: Build row-major matrix
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> all_vectors(num_vectors, dim);

    for (size_t i = 0; i < num_vectors; ++i) {
        std::cout << "Before move to matrix: append_store[" << i << "] shape = ["
                  << append_store[i].rows() << " x " << append_store[i].cols()
                  << "], values = " << append_store[i] << "\n";

        all_vectors.row(i) = std::move(append_store[i]); // move into matrix

        std::cout << "After move: append_store[" << i << "].cols() = " << append_store[i].cols() << "\n";
    }

    // Print final matrix
    std::cout << "\nFinal matrix to FAISS:\n" << all_vectors << "\n";

    // Step 3: FAISS index
    faiss::IndexHNSWFlat index(dim, 16);
    index.add(num_vectors, all_vectors.data());

    // Step 4: Validate
    std::cout << "\nFAISS index total vectors: " << index.ntotal << std::endl;
// Step 4: Perform search
    float query[3] = {2.0f, 3.0f, 4.0f}; // should match vector at index 2
    float distances[1];
    idx_t labels[1];

    index.search(1, query, 1, distances, labels);

    std::cout << "\nQuery: [";
    for (int i = 0; i < dim; ++i) std::cout << query[i] << (i < dim - 1 ? ", " : "");
    std::cout << "]\n";

    std::cout << "Nearest index: " << labels[0] << ", distance: " << distances[0] << "\n";

    // Step 5: Print matched vector from all_vectors
    if (labels[0] >= 0 && labels[0] < num_vectors) {
        std::cout << "Matched vector: " << all_vectors.row(labels[0]) << "\n";
    } else {
        std::cout << "Invalid match index!\n";
    }

    return 0;
}

/*
Before move: vec 0 shape = [1 x 3], values = 0 1 2
Before move: vec 1 shape = [1 x 3], values = 1 2 3
Before move: vec 2 shape = [1 x 3], values = 2 3 4
Before move: vec 3 shape = [1 x 3], values = 3 4 5
Before move: vec 4 shape = [1 x 3], values = 4 5 6
Before move to matrix: append_store[0] shape = [1 x 3], values = 0 1 2
After move: append_store[0].cols() = 3
Before move to matrix: append_store[1] shape = [1 x 3], values = 1 2 3
After move: append_store[1].cols() = 3
Before move to matrix: append_store[2] shape = [1 x 3], values = 2 3 4
After move: append_store[2].cols() = 3
Before move to matrix: append_store[3] shape = [1 x 3], values = 3 4 5
After move: append_store[3].cols() = 3
Before move to matrix: append_store[4] shape = [1 x 3], values = 4 5 6
After move: append_store[4].cols() = 3

Final matrix to FAISS:
0 1 2
1 2 3
2 3 4
3 4 5
4 5 6

FAISS index total vectors: 5

Query: [2, 3, 4]
Nearest index: 2, distance: 0
Matched vector: 2 3 4
*/