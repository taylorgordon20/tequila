#pragma once

#include <sqlite_modern_cpp.h>
#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>
#include <string>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"

namespace tequila {

template <typename ValueType>
inline auto serialize(const ValueType& value) {
  std::stringstream ss;
  cereal::BinaryOutputArchive archive(ss);
  archive(value);
  return ss.str();
}

template <typename ValueType, typename StringType>
inline auto deserialize(const StringType& data) {
  ValueType ret;
  std::stringstream ss;
  ss << data;
  cereal::BinaryInputArchive archive(ss);
  archive(ret);
  return ret;
}

class Table {
 public:
  Table(const std::string& name);

  bool has(const std::string& key);
  void del(const std::string& key);
  void set(const std::string& key, const std::string& data);
  std::string get(const std::string& key);

  template <typename T>
  void setObject(const std::string& key, const T& data) {
    set(key, serialize(data));
  }

  template <typename T>
  T getObject(const std::string& key) {
    return deserialize<T>(get(key));
  }

 private:
  sqlite::database db_;
};

}  // namespace tequila
