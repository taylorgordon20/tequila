#pragma once

#include <chrono>
#include <iostream>

#include "src/common/strings.hpp"

namespace tequila {

class Timer {
 public:
  Timer(const char* message)
      : message_(message), start_(std::chrono::high_resolution_clock::now()) {}
  ~Timer() {
    using namespace std::chrono;
    auto end = high_resolution_clock::now();
    auto dur = duration_cast<duration<double>>(end - start_).count();
    std::cout << format("Timer[%1%]=%2%", message_, dur) << std::endl;
  }

 private:
  const char* message_;
  std::chrono::high_resolution_clock::time_point start_;
};

}  // namespace tequila