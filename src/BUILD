package(default_visibility = ["//visibility:public"])
load("//:data.bzl", "static_files")

static_files(
  name = "static_files",
  srcs = [
    "//data:files",
    "//shaders:files",
    "//images:files",
  ],
)

cc_binary(
  name = "game",
  srcs = ["game.cpp"],
  data = [":static_files"],
  deps = [
    "//src/common:lib",
    "//src/worlds:lib",
    "//third_party:lib",
  ],
)