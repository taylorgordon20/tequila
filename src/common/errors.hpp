#pragma once

#include <boost/format.hpp>
#include <ctime>

namespace tequila {

template <typename StringType, typename... Args>
inline void throwError(StringType fmt, Args&&... args) {
  auto error = (boost::format(fmt) % ... % std::forward<Args>(args)).str();
  throw std::runtime_error(error.c_str());
}

#define ENFORCE(cond)                                                    \
  if (!(cond)) {                                                         \
    throwError(                                                          \
        "Failed condition '%1%' at %2%:%3%", #cond, __FILE__, __LINE__); \
  }

class ThrottledFn {
 public:
  ThrottledFn(float duration_s)
      : duration_s_(duration_s), calls_(0), ticks_(0), last_call_(0) {}

  template <typename FunctionType>
  void call(FunctionType fn) {
    ticks_ += 1;
    if (clock() - last_call_ > CLOCKS_PER_SEC * duration_s_) {
      calls_ += 1;
      fn(calls_, ticks_);
      last_call_ = clock();
      ticks_ = 0;
    }
  }

 private:
  float duration_s_;
  int64_t calls_;
  int64_t ticks_;
  clock_t last_call_;
};

#define THROTTLED_FN(duration_s, fn)                    \
  {                                                     \
    thread_local ThrottledFn __throttle_fn(duration_s); \
    __throttle_fn.call(std::move(fn));                  \
  }

}  // namespace tequila