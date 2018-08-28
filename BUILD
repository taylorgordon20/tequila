package(default_visibility = ["//visibility:public"])

cc_library(
  name = "window_system",
  srcs = [],
  linkopts = select({
    "@bazel_tools//src/conditions:windows": [
      "-DEFAULTLIB:opengl32",
      "-DEFAULTLIB:kernel32",
      "-DEFAULTLIB:user32",
      "-DEFAULTLIB:gdi32",
      "-DEFAULTLIB:shell32",
    ],
    "//conditions:default": [],
  })
)

cc_library(
  name = "third_party",
  deps = [
    "@glfw//:lib",
    "@eigen//:lib",
    "@zlib//:lib",
  ],
)