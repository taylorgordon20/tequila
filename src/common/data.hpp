#pragma once

#define SQLITE_THREADSAFE = 1

#include <sqlite_modern_cpp.h>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>
#include <string>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"

// Define custom serialization for maps keyed by strings since this use case
// occurs often and the default serialization is cumbersome.
namespace cereal {
template <typename Archive, typename Value>
inline void save(
    Archive& ar, const std::unordered_map<std::string, Value>& map) {
  for (const auto& pair : map) {
    ar(cereal::make_nvp(pair.first, pair.second));
  }
}

template <class Archive, typename Value>
inline void load(Archive& ar, std::unordered_map<std::string, Value>& map) {
  map.clear();
  Value value;
  for (auto name = ar.getNodeName(); name; name = ar.getNodeName()) {
    ar(value);
    map.emplace(name, std::move(value));
  }
}
}  // namespace cereal

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

class JsonParser {
 public:
  JsonParser(const std::string& json) {
    std::stringstream ss;
    ss << json;
    archive_ = std::make_unique<cereal::JSONInputArchive>(ss);
  }

  template <typename FieldType, typename StringType>
  FieldType get(StringType&& name) {
    FieldType field;
    (*archive_)(cereal::make_nvp(std::forward<StringType>(name), field));
    return field;
  }

  template <typename FieldType, typename StringType>
  void set(StringType&& name, FieldType& field) {
    (*archive_)(cereal::make_nvp(std::forward<StringType>(name), field));
  }

 private:
  std::unique_ptr<cereal::JSONInputArchive> archive_;
};

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

  JsonParser getJson(const std::string& key) {
    return JsonParser(get(key));
  }

 private:
  sqlite::database db_;
};

}  // namespace tequila
