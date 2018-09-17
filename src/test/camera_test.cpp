#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

#include "src/common/camera.hpp"
#include "src/common/spatial.hpp"
#include "src/common/timers.hpp"

namespace tequila {

TEST_CASE("Test visible cell algorithm", "[camera]") {
  using namespace Catch::Matchers;

  // Create the camera
  Camera camera;
  camera.position = glm::vec3(2048.0f, 2048.0f, 2048.0f);
  camera.view = glm::vec3(0.0f, 0.0f, 1.0f);
  camera.fov = glm::radians(45.0f);
  camera.aspect = 1.0f;
  camera.near_distance = 0.1f;
  camera.far_distance = 100.0f;

  // Size of the entire space is 32 * 256 = 2**12 = 4096.
  Octree octree(32, 256);
  auto cells_vec = [&] {
    Timer timer("computeVisibleCells");
    return computeVisibleCells(camera, octree);
  }();

  std::unordered_set<int64_t> cells_set;
  cells_set.insert(cells_vec.begin(), cells_vec.end());

  // Make sure that no cell's ancestor is in the returned set.
  for (auto cell : cells_vec) {
    for (; cell;) {
      cell = octree.cellParent(cell);
      REQUIRE(!cells_set.count(cell));
    }
  }

  // Make sure that the cell's combined volume compares well to the frustum.
  auto cells_volume = 0.0f;
  for (auto cell : cells_vec) {
    auto box = octree.cellBox(cell);
    auto width = std::get<3>(box) - std::get<0>(box);
    auto height = std::get<4>(box) - std::get<1>(box);
    auto depth = std::get<5>(box) - std::get<2>(box);
    cells_volume += height * width * depth;
  }
  REQUIRE(cells_volume >= frustumVolume(camera));
  REQUIRE(cells_volume <= 5 * frustumVolume(camera));
}

}  // namespace tequila