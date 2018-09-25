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
    ":chakracore",
    ":glbinding",
    ":glfw3",
    ":libpng",
    ":lua",
    ":sqlite3",
    ":zlib",
    ":zstd",
    "//:system_lib",
  ],
)

cc_import(
  name = "chakracore",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/Chakracore.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/ChakraCore.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "glbinding",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/glbinding.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/glbinding.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libglbinding.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "glfw3",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/glfw3dll.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/glfw3.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libglfw3.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "libpng",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/libpng16.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/libpng16.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libpng16.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "lua",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/lua.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/lua.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "sqlite3",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/sqlite3.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/sqlite3.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libsqlite3.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "zlib",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/zlib.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/zlib1.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libz.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "zstd",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/zstd.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/zstd.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libzstd.a",
    "//conditions:default": None,
  }),
)
