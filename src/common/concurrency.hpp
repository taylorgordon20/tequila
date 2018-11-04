#pragma once

#include <boost/optional.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

#include "src/common/errors.hpp"

namespace tequila {

class QueueExecutor {
 public:
  QueueExecutor(size_t thread_count) : running_(true) {
    ENFORCE(thread_count > 0);
    for (int i = 0; i < thread_count; i += 1) {
      workers_.emplace_back([&] {
        while (running_) {
          std::unique_lock<std::mutex> lock(mutex_);
          if (!tasks_.empty()) {
            auto task = tasks_.front();
            tasks_.pop();
            lock.unlock();
            task();
          } else {
            cv_.wait(lock);
          }
        }
      });
    }
  }

  ~QueueExecutor() {
    running_ = false;
    cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  template <typename Function>
  auto schedule(Function&& fn) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto promise = std::make_shared<std::promise<decltype(fn())>>();
    auto ret = promise->get_future();
    tasks_.push(makeTask(std::forward<Function>(fn), std::move(promise)));
    cv_.notify_all();
    return ret;
  }

 private:
  template <typename Function>
  auto makeTask(Function&& fn, std::shared_ptr<std::promise<void>>&& promise) {
    return [fn = std::forward<Function>(fn),
            promise = std::move(promise)]() mutable {
      try {
        fn();
        promise->set_value();
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    };
  }

  template <
      typename Function,
      typename PromiseType,
      typename = std::enable_if_t<!std::is_void_v<PromiseType>>>
  auto makeTask(
      Function&& fn, std::shared_ptr<std::promise<PromiseType>>&& promise) {
    return [fn = std::forward<Function>(fn),
            promise = std::move(promise)]() mutable {
      try {
        promise->set_value(fn());
      } catch (...) {
        promise->set_exception(std::current_exception());
      }
    };
  }

  bool running_;
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace tequila