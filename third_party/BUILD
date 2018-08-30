package(default_visibility = ["//visibility:public"])

# Maps bazel platform/architecture settings to their vcpkg triplet.
TRIPLET_MAP = {
  "@bazel_tools//src/conditions:windows": "x64-windows",
  "@bazel_tools//src/conditions:darwin": "x64-osx",
}

# Maps to identify per-triplet libraries and includes.
CONDITION_MAP = {v: k for k, v in TRIPLET_MAP.items()}
INCLUDE_MAP = {k: v + "/include" for k, v in TRIPLET_MAP.items()}

cc_library(
  name = "lib",
  hdrs = select({
    k: glob([v + "/**"]) for k, v in INCLUDE_MAP.items()
  }),
  includes = select({
    k: [v] for k, v in INCLUDE_MAP.items()
  }),
  deps = [
    ":glbinding",
    ":glfw3",
    ":sqlite3",
    ":zlib",
    ":zstd",
    "//:system_lib",
  ],
)

cc_import(
  name = "glbinding",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/glbinding.lib",
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/glbinding.dll",
  }),
)

cc_import(
  name = "glfw3",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/glfw3dll.lib",
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/glfw3.dll",
  }),
)

cc_import(
  name = "sqlite3",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/sqlite3.lib",
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/sqlite3.dll",
  }),
)

cc_import(
  name = "zlib",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/zlib.lib",
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/zlib1.dll",
  }),
)

cc_import(
  name = "zstd",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/zstd.lib",
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/zstd.dll",
  }),
)