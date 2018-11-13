#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "src/common/timers.hpp"

namespace tequila {

class Stats {
 public:
  void clear() {
    std::lock_guard lock(mutex_);
    stats_.clear();
    averages_.clear();
    maximums_.clear();
  }

  void set(const std::string& key, float value) {
    std::lock_guard lock(mutex_);
    if (stats_.insert(key).second) {
      averages_[key] = value;
      maximums_[key] = value;
    } else {
      averages_[key] = 0.9 * averages_[key] + 0.1 * value;
      maximums_[key] = std::max(maximums_[key], value);
    }
  }

  bool has(const std::string& key) {
    std::lock_guard lock(mutex_);
    return stats_.count(key);
  }

  float getAverage(const std::string& key) {
    std::lock_guard lock(mutex_);
    return averages_.at(key);
  }

  float getMaximum(const std::string& key) {
    std::lock_guard lock(mutex_);
    return maximums_.at(key);
  }

  std::unordered_set<std::string> keys() {
    std::lock_guard lock(mutex_);
    return stats_;
  };

 private:
  std::mutex mutex_;
  std::unordered_set<std::string> stats_;
  std::unordered_map<std::string, float> averages_;
  std::unordered_map<std::string, float> maximums_;
};

class StatsUpdate {
 public:
  StatsUpdate(std::shared_ptr<Stats> stats) : stats_(std::move(stats)) {}
  ~StatsUpdate() {
    for (const auto& pair : values_) {
      stats_->set(pair.first, pair.second);
    }
  }

  float& operator[](const std::string& key) {
    return values_[key];
  }

 private:
  std::shared_ptr<Stats> stats_;
  std::unordered_map<std::string, float> values_;
};

class StatsTimer {
 public:
  StatsTimer(std::shared_ptr<Stats> stats, const char* name)
      : stats_(stats), timer_(name, [this](const auto& msg, auto duration) {
          stats_[msg] = duration;
        }) {}

 private:
  StatsUpdate stats_;
  Timer timer_;
};

}  // namespace tequila