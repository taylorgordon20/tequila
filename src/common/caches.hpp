#pragma once

#include <algorithm>
#include <cstdlib>
#include <unordered_map>

#include "src/common/errors.hpp"

namespace tequila {

template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
class Cache {
 public:
  Cache(size_t capacity) : capacity_(capacity), access_tick_(0) {
    ENFORCE(capacity > 0);
  }

  bool has(const Key& key) const {
    return map_.count(key);
  }

  Value get(const Key& key) const {
    return map_.at(key).second;
  }

  void set(const Key& key, Value value) {
    map_[key] = std::make_pair(access_tick_++, std::move(value));
    if (map_.size() > capacity_) {
      prune();
    }
  }

  void del(const Key& key) {
    map_.erase(key);
  }

 private:
  using PartitionVector = std::vector<std::tuple<Key, Value, int64_t>>;

  void prune() {
    PartitionVector pv;
    pv.reserve(map_.size());
    for (auto&& pair : std::move(map_)) {
      pv.emplace_back(
          std::move(pair.first),
          std::move(pair.second.second),
          pair.second.first);
    }
    topEntries(pv, std::max<size_t>(1, capacity_ / 2));
    map_.clear();
    for (auto&& tup : std::move(pv)) {
      auto pair = std::make_pair(std::get<2>(tup), std::get<1>(tup));
      map_.emplace(std::get<0>(tup), std::move(pair));
    }
  }

  void topEntries(PartitionVector& pv, size_t size) {
    ENFORCE(pv.size() >= size);
    std::sort(pv.begin(), pv.end(), [](auto& tup1, auto& tup2) {
      return std::get<2>(tup1) >= std::get<2>(tup2);
    });
    pv.resize(size);
  }

  size_t capacity_;
  int64_t access_tick_;
  std::unordered_map<Key, std::pair<int64_t, Value>, Hash, KeyEqual> map_;
};

}  // namespace tequila