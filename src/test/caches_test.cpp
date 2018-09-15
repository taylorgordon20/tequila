#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "src/common/caches.hpp"
#include "src/common/errors.hpp"

namespace tequila {

TEST_CASE("Basic usage", "[caches]") {
  Cache<int, int> cache(5);
  cache.set(1, 11);
  cache.set(2, 22);
  cache.set(3, 33);
  cache.set(4, 44);
  cache.set(5, 55);
  REQUIRE_FALSE(cache.has(0));
  REQUIRE(cache.has(1));
  REQUIRE(cache.has(2));
  REQUIRE(cache.has(3));
  REQUIRE(cache.has(4));
  REQUIRE(cache.has(5));
  REQUIRE_FALSE(cache.has(6));
  REQUIRE(11 == cache.get(1));
  REQUIRE(22 == cache.get(2));
  REQUIRE(33 == cache.get(3));
  REQUIRE(44 == cache.get(4));
  REQUIRE(55 == cache.get(5));
  cache.set(6, 66);
  REQUIRE(66 == cache.get(6));
  REQUIRE(55 == cache.get(5));
  REQUIRE_FALSE(cache.has(1));
  REQUIRE_FALSE(cache.has(2));
  REQUIRE_FALSE(cache.has(3));
  REQUIRE_FALSE(cache.has(4));
  cache.set(7, 77);
  cache.set(8, 88);
  cache.set(9, 99);
  REQUIRE(55 == cache.get(5));
  REQUIRE(66 == cache.get(6));
  REQUIRE(77 == cache.get(7));
  REQUIRE(88 == cache.get(8));
  REQUIRE(99 == cache.get(9));
}

}  // namespace tequila