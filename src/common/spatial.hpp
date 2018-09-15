#pragma once

#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "src/common/errors.hpp"

namespace tequila {

// A data structure for storing run-length encoded vectors. The data structure
// is optimized for both space and speed. The cost of a get is proportional to
// a binary search of the number of ranges. The cost of a set is negligible if
// no ranges are updated, otherwise the cost is proportional to deque::insert.
template <typename ValueType>
class CompactVector {
 public:
  CompactVector(ValueType initial_value) {
    ranges_.push_back(std::make_pair(0, std::move(initial_value)));
  }

  ValueType get(int index) const {
    auto buffer_index = bisect(buffer_, index);
    if (buffer_index && buffer_.at(buffer_index - 1).first == index) {
      return buffer_.at(buffer_index - 1).second;
    }
    return ranges_.at(bisect(ranges_, index) - 1).second;
  }

  void set(int index, ValueType value) {
    // Return immediately if this index already reflect the given value.
    if (get(index) == value) {
      return;
    }

    // Insert the new pair into the buffer vector.
    for (auto iter = buffer_.begin();; ++iter) {
      if (iter == buffer_.end() || iter->first > index) {
        buffer_.insert(iter, std::make_pair(index, std::move(value)));
        break;
      } else if (iter->first == index) {
        iter->second = std::move(value);
        break;
      }
    }

    // If the buffer if full, update the ranges. The constant factor here was
    // empirically chosen to be approximately the fastest value.
    if (buffer_.size() > 4 * std::sqrt(ranges_.size())) {
      flush();
    }
  }

  template <typename Archive>
  void save(Archive& archive) const {
    archive(ranges_, buffer_);
  }

  template <typename Archive>
  void load(Archive& archive) {
    archive(ranges_, buffer_);
  }

 protected:
  auto flush() {
    if (buffer_.empty()) {
      return;
    }

    std::vector<std::pair<int, ValueType>> new_ranges;
    new_ranges.reserve(ranges_.size() + 2 * buffer_.size());

    auto index = 0;
    auto b_it = buffer_.begin();
    auto r_it = ranges_.begin();
    while (r_it != ranges_.end()) {
      if (b_it != buffer_.end() && b_it->first == index) {
        if (new_ranges.empty() || new_ranges.back().second != b_it->second) {
          new_ranges.emplace_back(index, std::move(b_it->second));
        }
        ++index;
        ++b_it;
      } else {
        auto r_next = r_it + 1;
        while (r_next != ranges_.end() && r_next->first <= index) {
          ++r_it;
          ++r_next;
        }
        if (new_ranges.empty() || new_ranges.back().second != r_it->second) {
          new_ranges.emplace_back(index, r_it->second);
        }
        if (b_it != buffer_.end()) {
          index = b_it->first;
          if (r_next != ranges_.end() && r_next->first < index) {
            index = r_next->first;
            r_it = r_next;
          }
        } else {
          ++r_it;
          index = r_it->first;
        }
      }
    }

    ranges_.swap(new_ranges);
    buffer_.clear();
  }

  auto bisect(const std::vector<std::pair<int, ValueType>>& v, int i) const {
    size_t lo = 0, hi = v.size();
    while (lo < hi) {
      auto mid = (lo + hi) / 2;
      if (v.at(mid).first > i) {
        hi = mid;
      } else {
        lo = mid + 1;
      }
    }
    return lo;
  }

 private:
  std::vector<std::pair<int, ValueType>> ranges_;
  std::vector<std::pair<int, ValueType>> buffer_;
};

template <size_t size, typename ValueType>
class SquareStore {
  static_assert(size <= 1 << 16, "Too big SquareStore size.");

 public:
  SquareStore(ValueType init) : cv_(std::move(init)) {}

  void set(int x, int y, ValueType value) {
    ENFORCE(0 <= x && x < size);
    ENFORCE(0 <= y && y < size);
    cv_.set(toIndex(x, y), std::move(value));
  }

  ValueType get(int x, int y) const {
    ENFORCE(0 <= x && x < size);
    ENFORCE(0 <= y && y < size);
    return cv_.get(toIndex(x, y));
  }

  size_t width() const {
    return size;
  }

  size_t height() const {
    return size;
  }

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(cv_);
  }

 private:
  int toIndex(int x, int y) const {
    return x + y * size;
  }

  CompactVector<ValueType> cv_;
};

template <size_t size, typename ValueType>
class CubeStore {
  static_assert(size <= 1 << 10, "Too big CubeStore size.");

 public:
  CubeStore(ValueType init) : cv_(std::move(init)) {}

  void set(int x, int y, int z, ValueType value) {
    ENFORCE(0 <= x && x < size);
    ENFORCE(0 <= y && y < size);
    ENFORCE(0 <= z && z < size);
    cv_.set(toIndex(x, y, z), std::move(value));
  }

  ValueType get(int x, int y, int z) const {
    ENFORCE(0 <= x && x < size);
    ENFORCE(0 <= y && y < size);
    ENFORCE(0 <= z && z < size);
    return cv_.get(toIndex(x, y, z));
  }

  size_t width() const {
    return size;
  }

  size_t height() const {
    return size;
  }

  size_t depth() const {
    return size;
  }

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(cv_);
  }

 private:
  int toIndex(int x, int y, int z) const {
    return x + y * size + z * size * size;
  }

  CompactVector<ValueType> cv_;
};

}  // namespace tequila