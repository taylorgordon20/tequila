package(default_visibility = ["//visibility:public"])
load("//:data.bzl", "static_files")

static_files(
  name = "test_data",
  srcs = [
    "//data:files",
    "//shaders:files",
    "//images:files",
  ],
)

cc_binary(
  name = "glfw_test",
  srcs = ["glfw_test.cpp"],
  data = [":test_data"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
  ],
)

cc_binary(
  name = "hello_world",
  srcs = ["hello_world.cpp"],
  deps = [
    "//third_party:lib",
  ],
)

cc_binary(
  name = "images_test",
  srcs = ["images_test.cpp"],
  data = [":test_data"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
  ],
)

cc_binary(
  name = "registry_test",
  srcs = ["registry_test.cpp"],
  deps = [
    "//src/common:lib"
  ],
)

cc_binary(
  name = "spatial_test",
  srcs = ["spatial_test.cpp"],
  data = [":test_data"],
  deps = [
    "//src/common:lib",
    "//third_party:lib",
  ],
)