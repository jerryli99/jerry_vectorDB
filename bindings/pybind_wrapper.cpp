#include <pybind11/stl.h>
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include "index/IndexFlat.h"
#include "index/Index.h"

namespace py = pybind11;
using namespace vector_db;

PYBIND11_MODULE(vector_db, m) {
    py::class_<Index, std::shared_ptr<Index>>(m, "Index");

    py::class_<IndexFlat, Index, std::shared_ptr<IndexFlat>>(m, "IndexFlat")
        .def(py::init<int>())
        .def("add", &IndexFlat::add)
        .def("search", &IndexFlat::search)
        .def("dimension", &IndexFlat::dimension)
        .def("get_all", &IndexFlat::get_all)
        .def("get_vector", &IndexFlat::get_vector);
}
