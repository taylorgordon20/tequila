#pragma once

#include <cassert>
#include <chrono>
#include <iostream>

#include "src/common/errors.hpp"
#include "src/common/strings.hpp"

namespace tequila {

inline auto defaultLogFn() {
  return [](const std::string& msg, double dur) {
    std::cout << format("wall_time[%1%]=%2%", msg, dur) << std::endl;
  };
}

class Timer {
 public:
  Timer(std::string message) : Timer(std::move(message), defaultLogFn()) {}

  template <typename Function>
  Timer(std::string message, Function&& log_fn)
      : log_fn_(std::forward<Function>(log_fn)) {
    begin(std::move(message));
  }

  ~Timer() {
    end();
    assert(tick_messages_.size() == tick_durations_.size());
    for (int i = 0; i < tick_messages_.size(); i += 1) {
      const auto& msg = tick_messages_.at(i);
      const auto dur = tick_durations_.at(i);
      log_fn_(msg, dur);
    }
  }

  void tick(std::string message) {
    end();
    begin(std::move(message));
  }

 private:
  void begin(std::string message) {
    tick_messages_.push_back(std::move(message));
    start_ = std::chrono::high_resolution_clock::now();
  }

  void end() {
    using namespace std::chrono;
    auto end = high_resolution_clock::now();
    auto dur = duration_cast<duration<double>>(end - start_).count();
    tick_durations_.push_back(dur);
  }

  std::function<void(const std::string&, double)> log_fn_;
  std::vector<std::string> tick_messages_;
  std::vector<double> tick_durations_;
  std::chrono::high_resolution_clock::time_point start_;
};

}  // namespace tequila