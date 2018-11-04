#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "src/common/concurrency.hpp"

namespace tequila {

TEST_CASE("Test queue executor", "[concurrency]") {
  std::atomic<int> counter(0);

  QueueExecutor executor(10);
  for (int i = 0; i < 10000; i += 1) {
    executor.schedule([&] { counter++; });
  }

  // Spin until all tasks are done.
  while (counter < 10000) {
  };

  // Make sure no further tasks execute.
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(100ms);
  REQUIRE(counter == 10000);
}

}  // namespace tequila