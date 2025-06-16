#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include "vector_db/brute_force_index.h"
#include "vector_db/index.h"

namespace py = pybind11;
using namespace vector_db;

PYBIND11_MODULE(vector_db, m) {
    py::class_<Index, std::shared_ptr<Index>>(m, "Index");

    py::class_<BruteForceIndex, Index, std::shared_ptr<BruteForceIndex>>(m, "BruteForceIndex")
        .def(py::init<int>())
        .def("add", &BruteForceIndex::add)
        .def("search", &BruteForceIndex::search)
        .def("dimension", &BruteForceIndex::dimension)
        .def("get_all", &BruteForceIndex::get_all)
        .def("get_vector", &BruteForceIndex::get_vector);
}
