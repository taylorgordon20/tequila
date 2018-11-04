#pragma once

#include <type_traits>

namespace tequila {

template <typename Function>
class Finally {
 public:
  Finally(Function&& fn) : fn_(std::forward<Function>(fn)) {}
  ~Finally() noexcept { fn_(); }

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

}  // namespace tequila