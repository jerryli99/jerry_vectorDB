#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include "vector_db.h"

namespace py = pybind11;

PYBIND11_MODULE(vector_db, m) {
    py::class_<VectorDB>(m, "VectorDB")
        .def(py::init<>())
        .def("add_vector", &VectorDB::add_vector,
             py::arg().noconvert())  // Disable implicit copying
        .def("get_vectors", &VectorDB::get_vectors)
        .def("add_vectors", &VectorDB::add_vectors)
        .def("subtract_vectors", &VectorDB::subtract_vectors);
}