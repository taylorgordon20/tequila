#pragma once

#include <unistd.h>
#include <fstream>
#include <streambuf>

#include "src/common/errors.hpp"

namespace tequila {

inline auto pathExists(const char* path) {
  std::ifstream ifs(path);
  return ifs.good();
}

inline auto resolvePathOrThrow(const char* relative_path) {
  // HACK: As a temporary hack to deal with the bazel run CWD override,
  // we also search the parent directory when resolving relative paths.
  std::string path(relative_path);
  if (!pathExists(path.c_str())) {
    path.insert(0, "../");
  }
  if (!pathExists(path.c_str())) {
    path.insert(0, "../");
  }
  ENFORCE(
      pathExists(path.c_str()),
      format("Unable to resolve path: %1%", relative_path));
  return path;
}

inline auto loadFile(const char* path) {
  std::ifstream ifs(resolvePathOrThrow(path).c_str());
  return std::string(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

}  // namespace tequila