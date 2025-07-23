#include <faiss/IndexFlat.h>
#include <faiss/Index.h>
#include <vector>
#include <iostream>
#include <iomanip>  // for formatted printing

// 64-bit int
using idx_t = faiss::idx_t;

int main() {
    using DenseVector = std::vector<float>;

    const int dim = 4;   // Dimension of each vector
    const int num_vecs = 10;
    const int k = 1;     // Top-k

    // Create FAISS index
    faiss::IndexFlatL2 index(dim);

    // Store all vectors in one flat array (row-major)
    std::vector<float> all_data;
    std::vector<DenseVector> original_vectors;

    for (int i = 0; i < num_vecs; ++i) {
        DenseVector vec(dim);
        for (int j = 0; j < dim; ++j) {
            vec[j] = static_cast<float>(i * 10 + j);  // e.g., [0,1,2,3], [10,11,12,13], ...
        }
        original_vectors.push_back(vec);
        all_data.insert(all_data.end(), vec.begin(), vec.end());
    }

    // Add to FAISS
    index.add(num_vecs, all_data.data());

    // Print the vectors as columns
    std::cout << "Vectors (column format):\n";
    for (int d = 0; d < dim; ++d) {
        for (int i = 0; i < num_vecs; ++i) {
            std::cout << std::setw(6) << original_vectors[i][d] << " ";
        }
        std::cout << "\n";
    }

    // Define query vector (manually or pick one of the inserted vectors)
    DenseVector query = {11.0f, 12.0f, 13.0f, 14.0f};

    // Search
    std::vector<idx_t> I(k);
    std::vector<float> D(k);
    index.search(1, query.data(), k, D.data(), I.data());

    // Print query
    std::cout << "\nQuery vector:\n";
    for (float x : query) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    // Print result
    std::cout << "\nTop-1 nearest neighbor index: " << I[0]
              << ", distance: " << D[0] << "\n";

    std::cout << "Matching vector:\n";
    for (float x : original_vectors[I[0]]) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    return 0;
}
