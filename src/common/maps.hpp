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

}  // namespace tequila