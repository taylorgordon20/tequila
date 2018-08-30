package(default_visibility = ["//visibility:public"])

cc_library(
  name = "lib",
  deps = [
    ":internal",
    "//:system_lib",
   ],
)

cc_library(
  name = "internal",
  srcs = select({
    "@bazel_tools//src/conditions:windows": glob([
      "x64-windows-static/lib/**/*.lib",
    ]),
    "@bazel_tools//src/conditions:darwin": glob([
      "x64-osx-static/lib/**/*.a",
    ]),
  }),
  hdrs = select({
    "@bazel_tools//src/conditions:windows": glob([
      "x64-windows-static/include/**",
    ]),
    "@bazel_tools//src/conditions:darwin": glob([
      "x64-osx-static/include/**",
    ]),
  }),
  strip_include_prefix = select({
    "@bazel_tools//src/conditions:windows": "x64-windows-static/include/",
    "@bazel_tools//src/conditions:darwin": "x64-osx-static/include/",
  }),
)