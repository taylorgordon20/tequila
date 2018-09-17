#pragma once

#include <boost/integer/integer_log2.hpp>
#include <cereal/types/unordered_map.hpp>
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

template <typename ValueType>
class SquareStore {
 public:
  SquareStore(size_t size, ValueType init) : size_(size), cv_(std::move(init)) {
    ENFORCE(size <= 1 << 16, "Too big SquareStore size.");
  }

  void set(int x, int y, ValueType value) {
    ENFORCE(0 <= x && x < size_);
    ENFORCE(0 <= y && y < size_);
    cv_.set(toIndex(x, y), std::move(value));
  }

  ValueType get(int x, int y) const {
    ENFORCE(0 <= x && x < size_);
    ENFORCE(0 <= y && y < size_);
    return cv_.get(toIndex(x, y));
  }

  size_t width() const {
    return size_;
  }

  size_t height() const {
    return size_;
  }

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(cv_);
  }

 private:
  int toIndex(int x, int y) const {
    return x + y * size_;
  }

  size_t size_;
  CompactVector<ValueType> cv_;
};

template <typename ValueType>
class CubeStore {
 public:
  CubeStore(size_t size, ValueType init) : size_(size), cv_(std::move(init)) {
    ENFORCE(size <= 1 << 10, "Too big CubeStore size.");
  }

  void set(int x, int y, int z, ValueType value) {
    ENFORCE(0 <= x && x < size_);
    ENFORCE(0 <= y && y < size_);
    ENFORCE(0 <= z && z < size_);
    cv_.set(toIndex(x, y, z), std::move(value));
  }

  ValueType get(int x, int y, int z) const {
    ENFORCE(0 <= x && x < size_);
    ENFORCE(0 <= y && y < size_);
    ENFORCE(0 <= z && z < size_);
    return cv_.get(toIndex(x, y, z));
  }

  size_t width() const {
    return size_;
  }

  size_t height() const {
    return size_;
  }

  size_t depth() const {
    return size_;
  }

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(cv_);
  }

 private:
  int toIndex(int x, int y, int z) const {
    return x + y * size_ + z * size_ * size_;
  }

  size_t size_;
  CompactVector<ValueType> cv_;
};

class Octree {
 public:
  using BoxTuple = std::tuple<int, int, int, int, int, int>;

  Octree(size_t leaf_size, size_t grid_size)
      : leaf_size_(leaf_size), grid_size_(grid_size) {
    ENFORCE(leaf_size);
    ENFORCE(grid_size);
    ENFORCE(grid_size < 1 << 20, "grid_size cannot exceed 2**21.")
    ENFORCE(!(grid_size & (grid_size - 1)), "grid_size must be power of 2.");
    cell_count_ = (grid_size_ * grid_size * grid_size_ * 8 - 1) / 7;
    tree_depth_ = boost::integer_log2(7 * cell_count_ + 1) / 3;
  }

  auto cellCount() const {
    return cell_count_;
  };

  auto treeDepth() const {
    return tree_depth_;
  };

  auto cellLevel(int64_t cell) const {
    return boost::integer_log2(7 * cell + 1) / 3;
  }

  auto cellParent(int64_t cell) const {
    ENFORCE(cell > 0);
    return (cell - 1) / 8;
  }

  template <typename CellFunction>
  auto search(CellFunction cell_fn, int64_t root_cell = 0) const {
    ENFORCE(0 <= root_cell && root_cell < cellCount());
    std::vector<int64_t> stack{root_cell};
    while (stack.size()) {
      auto cell = stack.back();
      stack.pop_back();
      if (cell_fn(cell) && 8 * cell + 1 < cellCount()) {
        for (int i = 0; i < 8; i += 1) {
          stack.push_back(8 * cell + 1 + i);
        }
      }
    }
  }

  // Returns the octree cell IDs intersecting the given bounding box.
  auto intersectBox(const BoxTuple& box) const {
    std::vector<int64_t> ret;
    search([&](int64_t cell) {
      auto tbox = cellBox(cell);
      auto xq = std::get<3>(box) - std::get<0>(tbox) - 1;
      auto xt = std::get<3>(tbox) - std::get<0>(box) - 1;
      auto yq = std::get<4>(box) - std::get<1>(tbox) - 1;
      auto yt = std::get<4>(tbox) - std::get<1>(box) - 1;
      auto zq = std::get<5>(box) - std::get<2>(tbox) - 1;
      auto zt = std::get<5>(tbox) - std::get<2>(box) - 1;
      if ((xq ^ xt) >= 0 && (yq ^ yt) >= 0 && (zq ^ zt) >= 0) {
        ret.push_back(cell);
        return true;
      }
      return false;
    });
    return ret;
  }

  // Returns a bounding box (in coordinate-pair format) for the given cell.
  BoxTuple cellBox(int64_t cell) const {
    int level = cellLevel(cell);
    int64_t ic = cell - ((1 << 3 * level) - 1) / 7;
    int ix = 0, iy = 0, iz = 0;
    for (int shift = 0; shift < level; shift += 1) {
      ix += (0b1 & ic) << shift;
      iy += (0b1 & (ic >> 1)) << shift;
      iz += (0b1 & (ic >> 2)) << shift;
      ic >>= 3;
    }
    int cell_size = (grid_size_ * leaf_size_) >> level;
    return std::make_tuple(
        ix * cell_size,
        iy * cell_size,
        iz * cell_size,
        (ix + 1) * cell_size,
        (iy + 1) * cell_size,
        (iz + 1) * cell_size);
  }

 private:
  size_t leaf_size_;
  size_t grid_size_;
  size_t tree_depth_;
  size_t cell_count_;
};

}  // namespace tequila