#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "src/common/errors.hpp"
#include "src/common/resources.hpp"

namespace tequila {

struct A {
  auto operator()(const Resources& resources) {
    return "A";
  }
};

struct B {
  auto operator()(const Resources& resources, int x) {
    return format("B%1%", x);
  }
};

struct C {
  std::string operator()(const Resources& resources, int x) {
    if (x > 0) {
      return format(
          "C%1%(%2%,%3%)", x, resources.get<B>(x), resources.get<C>(x - 1));
    } else {
      return "_";
    }
  }
};

struct D {
  auto operator()(const Resources& resources, const std::string& s, int t) {
    return format("D%1%%2%", s, t);
  }
};

struct E {
  auto operator()(const Resources& resources, const int x, const int y) {
    return format("E%1%%2%", x, y);
  }
};

struct F {
  auto operator()(const Resources& resources, const std::string& s) {
    return format("F%1%", s);
  }
};

auto not_F(const Resources& resources, const std::string& s) {
  return format("not_F%1%", s);
}

struct G : public SeedResource<G, std::string> {};

struct H {
  std::string operator()(const Resources& resources, int key) {
    static std::unordered_map<int, int> versions;
    return format(
        "H%1%.%2%(%3%,%4%)",
        key,
        versions[key]++,
        resources.get<B>(key),
        key > 0 ? resources.get<H>(key - 1) : "_");
  }
};

TEST_CASE("Test basic usage", "[resources]") {
  Resources resources;
  REQUIRE("A" == resources.get<A>());
  REQUIRE("B1" == resources.get<B>(1));
  REQUIRE("B2" == resources.get<B>(2));
  REQUIRE("C1(B1,_)" == resources.get<C>(1));
  REQUIRE("C2(B2,C1(B1,_))" == resources.get<C>(2));
}

TEST_CASE("Test overrides", "[resources]") {
  auto not_C = "not_C";
  auto not_D = [](const Resources& resources, const std::string& s, int t) {
    return format("not_D%1%%2%", s, t);
  };

  auto resources =
      ResourcesBuilder()
          .withOverride<A>([](const Resources& resources) { return "not_A"; })
          .withOverride<C>([&](const Resources& resources, int x) {
            return format("%1%%2%", not_C, x);
          })
          .withOverride<D>(std::move(not_D))
          .withSeed<E>("not_E")
          .withOverride<F>(not_F)
          .build();
  REQUIRE("not_A" == resources.get<A>());
  REQUIRE("B1" == resources.get<B>(1));
  REQUIRE("not_C1" == resources.get<C>(1));
  REQUIRE("not_C2" == resources.get<C>(2));
  REQUIRE("not_Dfoo2" == resources.get<D>("foo", 2));
  REQUIRE("not_Dfoo3" == resources.get<D>("foo", 3));
  REQUIRE("not_E" == resources.get<E>(1, 2));
  REQUIRE("not_Foo" == resources.get<F>("oo"));
}

TEST_CASE("Test seed resources", "[resources]") {
  auto g_func = [](const Resources&) -> std::string { return "good2"; };
  auto broken = ResourcesBuilder().build();
  auto okay_1 = ResourcesBuilder().withSeed<G>("good1").build();
  auto okay_2 = ResourcesBuilder().withOverride<G>(std::move(g_func)).build();
  REQUIRE_THROWS(broken.get<G>());
  REQUIRE("good1" == okay_1.get<G>());
  REQUIRE("good2" == okay_2.get<G>());
}

TEST_CASE("Test caching behavior", "[resources]") {
  Resources resources;
  REQUIRE("H2.0(B2,H1.0(B1,H0.0(B0,_)))" == resources.get<H>(2));
  REQUIRE("H2.0(B2,H1.0(B1,H0.0(B0,_)))" == resources.get<H>(2));
  resources.invalidate<H>(2);
  REQUIRE("H2.1(B2,H1.0(B1,H0.0(B0,_)))" == resources.get<H>(2));
  resources.invalidate<H>(1);
  REQUIRE("H2.2(B2,H1.1(B1,H0.0(B0,_)))" == resources.get<H>(2));
  REQUIRE("H2.2(B2,H1.1(B1,H0.0(B0,_)))" == resources.get<H>(2));
  resources.invalidate<B>(2);
  REQUIRE("H2.3(B2,H1.1(B1,H0.0(B0,_)))" == resources.get<H>(2));
  REQUIRE("H1.1(B1,H0.0(B0,_))" == resources.get<H>(1));
  resources.invalidate<B>(0);
  resources.invalidate<B>(1);
  resources.invalidate<B>(2);
  resources.invalidate<H>(1);
  REQUIRE("B0" == resources.get<B>(0));
  REQUIRE("H0.1(B0,_)" == resources.get<H>(0));
  REQUIRE("H1.2(B1,H0.1(B0,_))" == resources.get<H>(1));
  REQUIRE("H2.4(B2,H1.2(B1,H0.1(B0,_)))" == resources.get<H>(2));
  REQUIRE("H1.2(B1,H0.1(B0,_))" == resources.get<H>(1));
  REQUIRE("H2.4(B2,H1.2(B1,H0.1(B0,_)))" == resources.get<H>(2));
  resources.invalidate<H>(2);
  REQUIRE("H2.5(B2,H1.2(B1,H0.1(B0,_)))" == resources.get<H>(2));
}

}  // namespace tequila