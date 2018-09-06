#pragma once

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <streambuf>

#include "src/common/errors.hpp"

namespace tequila {

inline auto resolvePathOrThrow(std::string relative_path) {
  auto path = boost::filesystem::system_complete(relative_path);
  if (!boost::filesystem::exists(path)) {
    path = boost::filesystem::system_complete(relative_path.insert(0, "../"));
  }
  ENFORCE(
      boost::filesystem::exists(path),
      boost::format("Unable to resolve path: %1%") % relative_path);
  return path;
}

template <typename StringType>
inline auto loadFile(StringType&& path) {
  auto resolved_path = resolvePathOrThrow(std::forward<StringType>(path));
  boost::filesystem::ifstream ifs(resolved_path);
  return std::string(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

}  // namespace tequila