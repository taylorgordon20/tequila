package(default_visibility = ["//visibility:public"])

cc_library(
  name = "lib",
  srcs = glob(["*.cpp"]),
  hdrs = glob(["*.hpp"], exclude=["js.hpp"]),
  deps = ["//third_party:lib"]
)