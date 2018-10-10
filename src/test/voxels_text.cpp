#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <algorithm>
#include <iostream>
#include <random>

#include "src/common/voxels.hpp"

namespace tequila {

TEST_CASE("Basic usage", "[compact_vector]") {
  VoxelArray va;
  va.set(1, 1, 1, 1);
  va.set(1, 1, 2, 1);
  va.set(1, 1, 3, 1);
  va.set(1, 1, 4, 1);
  REQUIRE(va.get(1, 1, 0) == 0);
  REQUIRE(va.get(1, 1, 1) == 1);
  REQUIRE(va.get(1, 1, 2) == 1);
  REQUIRE(va.get(1, 1, 3) == 1);
  REQUIRE(va.get(1, 1, 4) == 1);
  REQUIRE(va.get(1, 1, 5) == 0);
}

}  // namespace tequila