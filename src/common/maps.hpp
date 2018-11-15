#pragma once

namespace tequila {

template <
    typename Map,
    typename Key = typename Map::key_type,
    typename Value = typename Map::mapped_type>
typename Map::mapped_type get_or(
    const Map& map, const Key& key, Value&& value) {
  auto iter = map.find(key);
  if (iter != map.end()) {
    return iter->second;
  }
  return typename Map::mapped_type(std::forward<Value>(value));
}

template <typename Map, typename Key = typename Map::key_type>
const typename Map::mapped_type* get_ptr(const Map& map, const Key& key) {
  auto iter = map.find(key);
  return iter != map.end() ? &iter->second : nullptr;
}

template <typename Map, typename Key = typename Map::key_type>
typename Map::mapped_type* get_ptr(Map& map, const Key& key) {
  auto iter = map.find(key);
  return iter != map.end() ? &iter->second : nullptr;
}

}  // namespace tequila