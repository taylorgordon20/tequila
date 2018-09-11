#include <pybind11/pybind11.h>

#include "src/common/data.hpp"

namespace py = pybind11;

namespace tequila {}  // namespace tequila

PYBIND11_MODULE(data, m) {
  using namespace tequila;
  m.doc() = "Routines for operating on pure data";
  py::class_<Table>(m, "Table")
      .def(py::init<const std::string&>())
      .def("has", &Table::has)
      .def("del", &Table::del)
      .def("set", &Table::set)
      .def("get", [](Table& self, const std::string& key) {
        return py::bytes(self.get(key));
      });
}
