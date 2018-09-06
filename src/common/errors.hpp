#pragma once

#include <boost/chrono.hpp>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>
#include <chrono>
#include <ctime>

namespace tequila {

template <typename StringType, typename... Args>
inline void throwError(StringType fmt, Args&&... args) {
  auto error = (boost::format(fmt) % ... % std::forward<Args>(args)).str();
  throw std::runtime_error(error.c_str());
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

#define __TIMER_DECL1(a, b) boost::timer::auto_cpu_timer a##b
#define __TIMER_DECL2(a, b) __TIMER_DECL1(a, b)
#define TIMER_GUARD(msg) \
  __TIMER_DECL2(__tg, __LINE__)("timer(" msg "): %ws wall time\n");

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