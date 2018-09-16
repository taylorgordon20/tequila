#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "src/common/spatial.hpp"

namespace py = pybind11;

namespace tequila {}  // namespace tequila

PYBIND11_MODULE(spatial, m) {
  using namespace tequila;
  m.doc() = "Routines for operating on spatial data structures";
  py::class_<Octree>(m, "Octree")
      .def(py::init<size_t, size_t>())
      .def("__len__", &Octree::cellCount)
      .def("depth", &Octree::treeDepth)
      .def("intersect_box", &Octree::intersectBox);
}
