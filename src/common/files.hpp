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

inline auto resolvePath(const std::string& relative_path) {
  // HACK: As a temporary hack to deal with the bazel run CWD override,
  // we also search the parent directory when resolving relative paths.
  boost::optional<std::string> ret;
  std::string path = relative_path;
  for (int i = 0; i < 2; i += 1) {
    if (pathExists(path)) {
      ret = path;
      break;
    } else {
      path.insert(0, "../");
    }
  }
  return ret;
}

inline auto resolvePathOrThrow(const std::string& relative_path) {
  if (auto opt_path = resolvePath(relative_path)) {
    return opt_path.get();
  }
  throwError(format("Unable to resolve path: %1%", relative_path));
}

inline auto loadFile(const std::string& path) {
  std::ifstream ifs(resolvePathOrThrow(path).c_str());
  return std::string(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

}  // namespace tequila