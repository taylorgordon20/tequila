#pragma once

#include "src/common/strings.hpp"

namespace tequila {

template <typename StringType, typename... Args>
[[noreturn]] inline void throwError(StringType fmt, Args&&... args) {
  throw std::runtime_error(format(fmt, std::forward<Args>(args)...).c_str());
}

#define __ENFORCE_1(cond)                                                \
  if (!(cond)) {                                                         \
    throwError(                                                          \
        "Failed condition '%1%' at %2%:%3%", #cond, __FILE__, __LINE__); \
  }

#define __ENFORCE_2(cond, msg)                                   \
  if (!(cond)) {                                                 \
    throwError(                                                  \
        "Failed condition '%1%' at %2%:%3%. Description: '%4%'", \
        #cond,                                                   \
        __FILE__,                                                \
        __LINE__,                                                \
        msg);                                                    \
  }

#define __WHICH_ENFORCE(_1, _2, NAME, ...) NAME
#define __MSVC_EXPAND(x) x
#define ENFORCE(...) \
  __MSVC_EXPAND(     \
      __WHICH_ENFORCE(__VA_ARGS__, __ENFORCE_2, __ENFORCE_1)(__VA_ARGS__))

#define LOGV(expr) std::cout << #expr << "=" << expr << std::endl;

}  // namespace tequila