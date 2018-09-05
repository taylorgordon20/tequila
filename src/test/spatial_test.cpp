#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <algorithm>
#include <iostream>
#include <random>

#include "src/common/images.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

TEST_CASE("Basic usage", "[compact_vector]") {
  CompactVector<int> compact_vector(0);
  compact_vector.set(3, 1);
  compact_vector.set(2, 1);
  compact_vector.set(5, 1);
  compact_vector.set(6, 1);
  compact_vector.set(7, 1);
  compact_vector.set(8, 1);

  REQUIRE(compact_vector.get(0) == 0);
  REQUIRE(compact_vector.get(1) == 0);
  REQUIRE(compact_vector.get(2) == 1);
  REQUIRE(compact_vector.get(3) == 1);
  REQUIRE(compact_vector.get(4) == 0);
  REQUIRE(compact_vector.get(5) == 1);
  REQUIRE(compact_vector.get(6) == 1);
  REQUIRE(compact_vector.get(7) == 1);
  REQUIRE(compact_vector.get(8) == 1);
  REQUIRE(compact_vector.get(9) == 0);
}

TEST_CASE("Test many random insertions", "[compact_vector]") {
  constexpr auto kNumElements = 100000;
  std::random_device rd;
  std::mt19937 rg(rd());

  std::vector<std::pair<int, int>> actual;
  for (int i = 0; i < kNumElements; i += 1) {
    actual.emplace_back(i, rg() % 5);
  }
  std::shuffle(actual.begin(), actual.end(), rg);

  CompactVector<int> compact_vector(0);
  for (const auto &pair : actual) {
    compact_vector.set(pair.first, pair.second);
  }

  for (const auto &pair : actual) {
    REQUIRE(compact_vector.get(pair.first) == pair.second);
  }
}

TEST_CASE("Test image RLE encoding", "[compact_vector]") {
  auto pixels = loadPngToTensor("images/spatial_test.png");
  ENFORCE(pixels.dimension(0) == pixels.dimension(1));

  // Compress the image into a compact RLE-encoded vector.
  SquareStore<int32_t> square_store(pixels.dimension(0), 0);
  for (int y = 0; y < pixels.dimension(1); y += 1) {
    for (int x = 0; x < pixels.dimension(0); x += 1) {
      int32_t rgb = 0;
      rgb |= pixels(x, y, 0);
      rgb |= pixels(x, y, 0) << 8;
      rgb |= pixels(x, y, 0) << 16;
      square_store.set(x, y, rgb);
    }
  }

  // Read the compressed pixels back into the tensor.
  pixels.setZero();
  for (int y = 0; y < pixels.dimension(1); y += 1) {
    for (int x = 0; x < pixels.dimension(0); x += 1) {
      int32_t rgb = square_store.get(x, y);
      pixels(x, y, 0) = rgb & 255;
      pixels(x, y, 1) = (rgb >> 8) & 255;
      pixels(x, y, 2) = (rgb >> 16) & 255;
    }
  }

  saveTensorToPng("spatial_test.png", pixels);
}

}  // namespace tequila