// #include <pybind11/stl.h>
// #include <pybind11/pybind11.h>
// #include <pybind11/eigen.h>
// #include "index/IndexFlat.h"
// #include "index/Index.h"

// namespace py = pybind11;
// using namespace vector_db;

// PYBIND11_MODULE(vector_db, m) {
//     py::class_<Index, std::shared_ptr<Index>>(m, "Index");

//     py::class_<IndexFlat, Index, std::shared_ptr<IndexFlat>>(m, "IndexFlat")
//         .def(py::init<int>())
//         .def("add", &IndexFlat::add)
//         .def("search", &IndexFlat::search)
//         .def("dimension", &IndexFlat::dimension)
//         .def("get_all", &IndexFlat::get_all)
//         .def("get_vector", &IndexFlat::get_vector);
// }

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <Eigen/Dense>
#include "index/IndexFlat.h"
#include "index/Index.h"
#include "VectorOps.h"

namespace py = pybind11;
using namespace vector_db;

PYBIND11_MODULE(vector_db, m) {
    // Core index bindings
    py::class_<Index, std::shared_ptr<Index>>(m, "Index");

    py::class_<IndexFlat, Index, std::shared_ptr<IndexFlat>>(m, "IndexFlat")
        .def(py::init<int>())
        .def("add", &IndexFlat::add)
        .def("search", &IndexFlat::search)
        .def("dimension", &IndexFlat::dimension)
        .def("get_all", &IndexFlat::get_all)
        .def("get_vector", &IndexFlat::get_vector);

    // Add vector operations namespace
    py::module_ ops = m.def_submodule("ops", "Vector operations");

    // Normalization
    ops.def("normalize", [](Eigen::MatrixXf& vectors, bool inplace) {
        if (inplace) {
            VectorOps::normalize<float, true>(vectors);
            return vectors;
        } else {
            return VectorOps::normalize<float, false>(vectors);
        }
    }, py::arg("vectors"), py::arg("inplace") = false);

    // Distance metrics
    ops.def("cosine_distance", &VectorOps::cosineDistance<float>,
           "Cosine distance for normalized vectors");
    
    ops.def("squared_euclidean", &VectorOps::squaredEuclidean<float>,
           "Squared Euclidean for normalized vectors");
    
    ops.def("inner_product", &VectorOps::innerProduct<float>,
           "Inner product (cosine similarity when normalized)");

    // Quantization methods
    ops.def("scalar_quantize", &VectorOps::scalarQuantize,
           "Convert FP32 vector to INT8");

    ops.def("binary_quantize", &VectorOps::binaryQuantize,
           "Convert vector to binary bits");

    ops.def("product_quantize", [](const Eigen::MatrixXf& vectors, int m, int k) {
        auto [codebooks, codes] = VectorOps::productQuantize(vectors, m, k);
        return py::make_tuple(codebooks, codes);
    }, py::arg("vectors"), py::arg("m") = 8, py::arg("k") = 256);
}