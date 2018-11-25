#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>

#include "src/common/concurrency.hpp"
#include "src/common/errors.hpp"
#include "src/common/opengl.hpp"
#include "src/common/registry.hpp"
#include "src/common/stats.hpp"
#include "src/common/window.hpp"

namespace tequila {

static constexpr auto kProcessThrottleDuration = std::chrono::milliseconds(5);

class OpenGLContextExecutor {
 public:
  OpenGLContextExecutor(
      std::shared_ptr<Stats> stats, std::shared_ptr<Window> window)
      : stats_(std::move(stats)), window_(std::move(window)) {}

  template <typename Function>
  auto manage(Function&& fn) {
    return runInOpenGLContext([&] {
      using Value = std::decay_t<decltype(*fn())>;
      return std::shared_ptr<Value>(fn(), [this](Value* obj) {
        runInOpenGLContext([obj] { delete obj; });
      });
    });
  }

  template <typename Function>
  auto runInOpenGLContext(Function&& fn) {
    if (window_->inContext()) {
      return fn();
    } else {
      std::promise<decltype(fn())> promise;
      auto future = promise.get_future();
      queue_.push(makeTask(promise, std::forward<Function>(fn)));
      return future.get();
    }
  }

  void process() {
    StatsTimer process_timer(stats_, "process_gl_tasks");
    ENFORCE(window_->inContext());
    auto prev = std::chrono::high_resolution_clock::now();
    while (!queue_.isEmpty()) {
      auto task = queue_.pop().get();
      {
        StatsTimer task_timer(stats_, "gl_task");
        task();
      }
      auto curr = std::chrono::high_resolution_clock::now();
      if (curr - prev > kProcessThrottleDuration) {
        break;
      }
    }
  }

  bool isEmpty() {
    return queue_.isEmpty();
  }

 private:
  template <typename Function>
  auto makeTask(std::promise<void>& promise, Function&& fn) {
    return [&] {
      fn();
      promise.set_value();
    };
  }

  template <typename PromiseType, typename Function>
  auto makeTask(std::promise<PromiseType>& promise, Function&& fn) {
    return [&] { promise.set_value(fn()); };
  }

  std::shared_ptr<Stats> stats_;
  std::shared_ptr<Window> window_;
  MPMCQueue<std::function<void()>> queue_;
};

template <>
inline std::shared_ptr<OpenGLContextExecutor> gen(const Registry& registry) {
  return std::make_shared<OpenGLContextExecutor>(
      registry.get<Stats>(), registry.get<Window>());
}

}  // namespace tequila
