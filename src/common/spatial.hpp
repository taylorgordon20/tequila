#pragma once

namespace tequila {

template <typename ValueType>
class CompactQuadTree {
 public:
  void set(int x, int y, ValueType&& value);
  const ValueType& get(int x, int y) const;
};

}  // namespace tequila