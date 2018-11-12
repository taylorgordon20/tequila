#include "src/common/errors.hpp"

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

#include "src/common/strings.hpp"

namespace tequila {

void logError(const std::string& message) {
  static std::mutex mutex;
  static std::unordered_map<std::thread::id, int> thread_index;
  std::lock_guard lock(mutex);
  if (!thread_index.count(std::this_thread::get_id())) {
    thread_index[std::this_thread::get_id()] = thread_index.size();
  }
  auto thread_id = thread_index.at(std::this_thread::get_id());
  std::stringstream ss;
  ss << "THREAD[" << thread_id << "]: " << message << std::endl;
  std::cout << ss.str();
}

}  // namespace tequila