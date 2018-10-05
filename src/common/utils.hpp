#pragma once

#include <type_traits>

namespace tequila {

template <typename T, typename = std::enable_if_t<!std::is_void_v<T>>>
inline auto makeDefault() {
  return T();
}

template <typename T, typename = std::enable_if_t<std::is_void_v<T>>>
inline void makeDefault() {
  return;
}

}  // namespace tequila