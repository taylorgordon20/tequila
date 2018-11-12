#pragma once

#include <iostream>
#include <tuple>
#include <type_traits>

namespace tequila {

template <typename Function>
class Finally {
 public:
  Finally(Function&& fn) : fn_(std::forward<Function>(fn)) {}
  ~Finally() noexcept {
    fn_();
  }

 private:
  Function fn_;
};

template <typename T, typename = std::enable_if_t<!std::is_void_v<T>>>
inline auto makeDefault() {
  return T();
}

template <typename T, typename = std::enable_if_t<std::is_void_v<T>>>
inline void makeDefault() {
  return;
}

template <size_t n, typename... T>
inline typename std::enable_if<(n >= sizeof...(T))>::type printTuple(
    std::ostream&, const std::tuple<T...>&) {}

template <size_t n, typename... T>
inline typename std::enable_if<(n < sizeof...(T))>::type printTuple(
    std::ostream& os, const std::tuple<T...>& tup) {
  if (n > 0) {
    os << ", ";
  }
  os << std::get<n>(tup);
  printTuple<n + 1>(os, tup);
}

template <typename... T>
inline std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& tup) {
  os << "[";
  printTuple<0>(os, tup);
  return os << "]";
}

}  // namespace tequila