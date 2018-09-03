package(default_visibility = ["//visibility:public"])
load("//:data.bzl", "static_files")

static_files(
  name = "test_data",
  srcs = ["//shaders:glsl_files"],
)

cc_binary(
  name = "glfw_test",
  srcs = ["glfw_test.cpp"],
  data = [
    ":test_data",
  ],
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
  name = "registry_test",
  srcs = ["registry_test.cpp"],
  deps = [
    "//src/common:lib"
  ],
)