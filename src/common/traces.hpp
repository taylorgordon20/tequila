#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace tequila {

using TraceTime = std::chrono::time_point<std::chrono::system_clock>;
using TraceTag = std::tuple<std::string, TraceTime>;

class Trace {
 public:
  template <typename Function>
  Trace(Function&& fn) : fn_(std::forward<Function>(fn)) {
    push(this);
    addTag("start");
  }

  ~Trace() {
    pop();
    addTag("finish");
    fn_(tags_);
  }

  void addTag(std::string key) {
    tags_.emplace_back(std::move(key), std::chrono::system_clock::now());
  }

  // Static functions that apply to the current thread trace stack.
  static void push(Trace* trace);
  static void pop();
  static void tag(std::string key);

 private:
  std::function<void(std::vector<TraceTag>&)> fn_;
  std::vector<TraceTag> tags_;
};

class ScopeTrace {
 public:
  ScopeTrace(std::string key) {
    Trace::tag(std::move(key));
  }
  ~ScopeTrace() {
    Trace::tag("end_scope");
  }
};

}  // namespace tequila