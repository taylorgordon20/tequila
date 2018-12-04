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
    ":boost-math",
    ":bz2",
    ":chakracore",
    ":flac",
    ":flac++",
    ":freetype",
    ":glbinding",
    ":glbinding-aux",
    ":glfw3",
    ":libpng",
    ":lua",
    ":ogg",
    ":openal",
    ":sfml-audio",
    ":sfml-system",
    ":sqlite3",
    ":vorbis",
    ":vorbisenc",
    ":vorbisfile",
    ":zlib",
    ":zstd",
    "//:system_lib",
  ],
)

cc_library(
  name = "boost-math",
  deps = [
    ":boost_atomic",
    ":boost_math_c99",
    ":boost_math_c99f",
    ":boost_math_c99l",
    ":boost_math_tr1",
    ":boost_math_tr1f",
    ":boost_math_tr1l",
  ],
)

cc_import(
  name = "boost_atomic",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_atomic-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_atomic-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_atomic.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_c99",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_c99-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_c99-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_c99.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_c99f",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_c99f-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_c99f-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_c99f.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_c99l",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_c99l-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_c99l-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_c99l.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_tr1",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_tr1-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_tr1-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_tr1.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_tr1f",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_tr1f-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_tr1f-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_tr1f.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "boost_math_tr1l",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/boost_math_tr1l-vc140-mt.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/boost_math_tr1l-vc141-mt-x64-1_68.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libboost_math_tr1l.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "bz2",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/bz2.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/libbz2.dll",
    "//conditions:default": None,
  }),
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
  name = "flac",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/FLAC.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/FLAC.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "flac++",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/FLAC++.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/FLAC.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "freetype",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/freetype.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/freetype.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libfreetype.a",
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
  name = "glbinding-aux",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/glbinding-aux.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/glbinding-aux.dll",
    "//conditions:default": None,
  }),
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/libglbinding-aux.a",
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
  static_library = select({
    CONDITION_MAP["x64-osx"]: "x64-osx/lib/liblua.a",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "ogg",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/ogg.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/ogg.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "openal",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/OpenAL32.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/OpenAL32.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "sfml-audio",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/sfml-audio.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/sfml-audio-2.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "sfml-system",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/sfml-system.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/sfml-system-2.dll",
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
  name = "vorbis",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/vorbis.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/vorbis.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "vorbisenc",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/vorbisenc.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/vorbisenc.dll",
    "//conditions:default": None,
  }),
)

cc_import(
  name = "vorbisfile",
  interface_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/lib/vorbisfile.lib",
    "//conditions:default": None,
  }),
  shared_library = select({
    CONDITION_MAP["x64-windows"]: "x64-windows/bin/vorbisfile.dll",
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
