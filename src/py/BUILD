package(default_visibility = ["//visibility:public"])
load("//src/py:extensions.bzl", "py_cpp_module", "py_extension")

py_cpp_module(
  name = "data",
  srcs = ["data_ext.cpp"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
    "@python//:include",
  ],
)

py_cpp_module(
  name = "spatial",
  srcs = ["spatial_ext.cpp"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
    "@python//:include",
  ],
)

py_cpp_module(
  name = "voxels",
  srcs = ["voxels_ext.cpp"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
    "@python//:include",
  ],
)

# To use, you'll need to add "blaze-bin/src/py" to your python path:
# import sys; sys.path.append("blaze-bin/src/py")
py_extension(
  name="tequila",
  version="0.0",
  module_base="src/py",
  modules=[
    ":data",
    ":spatial",
    ":voxels",
  ],
)