#include <pybind11/pybind11.h>
#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include "src/common/voxels.hpp"

namespace py = pybind11;

namespace tequila {

auto dumps(VoxelArray& voxels) {
  std::stringstream ss;
  cereal::BinaryOutputArchive archive(ss);
  archive(voxels);
  return py::bytes(ss.str());
}

auto loads(const std::string& data) {
  VoxelArray ret;
  std::stringstream ss;
  ss << data;
  cereal::BinaryInputArchive archive(ss);
  archive(ret);
  return ret;
}

}  // namespace tequila

PYBIND11_MODULE(voxels, m) {
  using namespace tequila;
  m.doc() = "Routines for operating on spatial data structures";
  py::class_<VoxelArray>(m, "VoxelArray")
      .def(py::init<>())
      .def("del", &VoxelArray::del)
      .def("get", &VoxelArray::get)
      .def("set", &VoxelArray::set);
  m.def("dumps", &dumps);
  m.def("loads", &loads);
}
