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
    ":boost_chrono",
    ":boost_container",
    ":boost_filesystem",
    ":boost_regex",
    ":boost_system",
    ":boost_timer",
    ":glbinding",
    ":glfw3",
    ":libpng",
    ":sqlite3",
    ":zlib",
    ":zstd",
    "//:system_lib",
  ],
)

cc_import(
  name = "boost_chrono",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_chrono-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_chrono-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_container",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_container-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_container-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_filesystem",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_filesystem-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_filesystem-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_regex",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_regex-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_regex-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_system",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_system-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_system-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_timer",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_timer-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_timer-vc141-mt-x64-1_68.dll",
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
