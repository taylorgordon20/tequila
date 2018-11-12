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

// Returns an optional set to the future's value if and only if it is ready.
inline bool get_opt(std::future<void>& future) {
  bool ret = false;
  if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    ret = true;
  }
  return ret;
}

// Returns an optional set to the future's value if and only if it is ready.
template <typename Value>
inline boost::optional<Value> get_opt(std::future<Value>& future) {
  boost::optional<Value> ret;
  if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    ret = future.get();
  }
  return ret;
}

// Executes the given function in a loop until the given future is ready.
template <typename Function>
inline void spin(std::future<void>& future, Function&& fn) {
  while (!get_opt(future)) {
    fn();
  }
}

template <typename Future, typename Function>
inline auto spin(Future& future, Function&& fn) {
  auto ret = get_opt(future);
  while (!(ret = get_opt(future))) {
    fn();
  }
  return *ret;
}

template <typename Value>
class MPMCQueue {
 public:
  MPMCQueue() : closed_(false) {}

  bool isOpen() {
    std::lock_guard lock(mutex_);
    return !closed_;
  }

  bool isEmpty() {
    std::lock_guard lock(mutex_);
    return queue_.empty();
  }

  void close() {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
      queue_.clear();
    }
    cv_.notify_all();
  }

  void push(Value value) {
    {
      std::lock_guard lock(mutex_);
      ENFORCE(!closed_);
      queue_.push_back(std::move(value));
    }
    cv_.notify_one();
  }

  boost::optional<Value> pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    boost::optional<Value> ret;
    while (!closed_) {
      if (!queue_.empty()) {
        ret = std::move(queue_.front());
        queue_.pop_front();
        break;
      } else {
        cv_.wait(lock);
      }
    }
    return ret;
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<Value> queue_;
  bool closed_;
};

class QueueExecutor {
 public:
  QueueExecutor(size_t thread_count) : finished_workers_(0) {
    ENFORCE(thread_count > 0);
    for (int i = 0; i < thread_count; i += 1) {
      workers_.emplace_back([&] {
        while (auto task = task_queue_.pop()) {
          (*task)();
        }
        finished_workers_ += 1;
      });
    }
  }

  ~QueueExecutor() {
    task_queue_.close();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  bool isDone() {
    return workers_.size() == finished_workers_;
  }

  void close() {
    return task_queue_.close();
  }

  template <typename Function>
  auto schedule(Function&& fn) {
    ENFORCE(task_queue_.isOpen());
    auto promise = std::make_shared<std::promise<decltype(fn())>>();
    auto ret = promise->get_future();
    task_queue_.push(makeTask(std::forward<Function>(fn), std::move(promise)));
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

  std::vector<std::thread> workers_;
  MPMCQueue<std::function<void()>> task_queue_;
  std::atomic<int> finished_workers_;
};

}  // namespace tequila