#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tequila {

class Stats {
 public:
  void set(const std::string& key, float value) {
    std::lock_guard lock(mutex_);
    if (stats_.insert(key).second) {
      averages_[key] = value;
    } else {
      averages_[key] = 0.9 * averages_[key] + 0.1 * value;
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

  std::unordered_set<std::string> keys() {
    std::lock_guard lock(mutex_);
    return stats_;
  };

 private:
  std::mutex mutex_;
  std::unordered_set<std::string> stats_;
  std::unordered_map<std::string, float> averages_;
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

}  // namespace tequila