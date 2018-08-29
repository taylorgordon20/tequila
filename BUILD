package(default_visibility = ["//visibility:public"])

cc_library(
  name = "system_lib",
  linkopts = select({
    "@bazel_tools//src/conditions:windows": [
      "-DEFAULTLIB:opengl32",
      "-DEFAULTLIB:kernel32",
      "-DEFAULTLIB:user32",
      "-DEFAULTLIB:gdi32",
      "-DEFAULTLIB:shell32",
    ],
    "@bazel_tools//src/conditions:darwin": [
      "-framework OpenGL",
      "-framework Cocoa",
      "-framework IOKit",
      "-framework CoreVideo",
    ],
    "//conditions:default": [],
  }),
  deps = [
    ":third_party",
  ]
)

cc_library(
  name = "third_party",
  deps = [
    "@boost//:lib",
    "@glew//:lib",
    "@glfw//:lib",
    "@eigen//:lib",
    "@zlib//:lib",
  ],
)