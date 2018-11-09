#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "src/common/concurrency.hpp"
#include "src/common/errors.hpp"
#include "src/common/opengl.hpp"
#include "src/common/registry.hpp"

namespace tequila {

class OpenGLContextExecutor {
 public:
  OpenGLContextExecutor(std::shared_ptr<Window> window)
      : window_(std::move(window)) {}

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
    // TODO: Add throttling / time-slicing to obviate frame stalls.
    ENFORCE(window_->inContext());
    while (!queue_.isEmpty()) {
      queue_.pop().get()();
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

  std::shared_ptr<Window> window_;
  MPMCQueue<std::function<void()>> queue_;
};

template <>
inline std::shared_ptr<OpenGLContextExecutor> gen(const Registry& registry) {
  return std::make_shared<OpenGLContextExecutor>(registry.get<Window>());
}

}  // namespace tequila
