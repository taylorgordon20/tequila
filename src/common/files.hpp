#pragma once

#include <sys/stat.h>
#include <fstream>
#include <streambuf>

#include "src/common/errors.hpp"
#include "src/common/strings.hpp"

namespace tequila {

inline auto pathExists(const std::string& path) {
  struct stat info;
  return !stat(path.c_str(), &info);
}

inline auto resolvePathOrThrow(const std::string& relative_path) {
  // HACK: As a temporary hack to deal with the bazel run CWD override,
  // we also search the parent directory when resolving relative paths.
  std::string path = relative_path;
  if (!pathExists(path)) {
    path.insert(0, "../");
  }
  if (!pathExists(path)) {
    path.insert(0, "../");
  }
  ENFORCE(
      pathExists(path), format("Unable to resolve path: %1%", relative_path));
  return path;
}

inline auto loadFile(const char* path) {
  std::ifstream ifs(resolvePathOrThrow(path).c_str());
  return std::string(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

}  // namespace tequila