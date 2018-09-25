#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "src/common/js.hpp"

namespace tequila {

constexpr auto kScript1 = R"(
var module = {};

module.foo = function() {
  return "foo_string";
};

module.bar = function(a_number) {
  return "bar_string:" + (2 * a_number);
};

module.two_args = function(a_number, a_string) {
  return "two_args:" + (3 * a_number) + a_string;
};

module.four_args = function(a_number, a_bool, a_string, a_int) {
  return "four_args:" + a_number + ":" + a_bool + ":" + a_string + ":" + a_int;
};

module.array_args = function(number_array) {
  var ret = "";
  for (var i = 0; i < number_array.length; i += 1) {
    ret += number_array[i];
  }
  return ret;
};

module;
)";

constexpr auto kScript2 = R"(
var module = {};

module.add = function(num1, num2) {
  return num1 + num2;
}

module.mul = function(num1, num2) {
  return num1 * num2;
}
 
module.arrcat = function(num1, num2, opt_num3) {
  if (typeof opt_num3 !== 'undefined') {
    return [num1, num2, opt_num3];
  } else {
    return [num1, num2];
  }
}

module.strcat = function(num1, num2) {
  return ("" + num1) + num2;
}

module;
)";

constexpr auto kScript3 = R"(
var module = {};

module.get_a_number = function() {
  return a_number;
}

module.get_a_bool = function() {
  return a_bool;
}

module.get_an_int = function() {
  return an_int;
}

module.get_a_string = function() {
  return a_string;
}

module.get_an_array = function() {
  return an_array;
}

module.get_a_map = function() {
  return a_map;
}

module;
)";

constexpr auto kScript4 = R"(
var module = {};

module.invoke1 = function(a) {
  return fn1(a);
}

module.invoke2 = function(a, b) {
  return fn2(a, b);
}

module.invoke3 = function(a, b, c) {
  return fn3(a, b, c);
}

module;
)";

TEST_CASE("Test argument types", "[js]") {
  JsModule module(std::make_shared<JsContext>(), "script_1", kScript1);
  REQUIRE("foo_string" == module.call<std::string>("foo"));
  REQUIRE("bar_string:6" == module.call<std::string>("bar", 3.0));
  REQUIRE(
      "two_args:9dogs" == module.call<std::string>("two_args", 3.0, "dogs"));
  REQUIRE(
      "four_args:-1:true:lies:42" ==
      module.call<std::string>("four_args", -1.0, true, "lies", 42));
  REQUIRE(
      "123456" == module.call<std::string>(
                      "array_args", std::vector<double>{1, 2, 3, 4, 5, 6}));
}

TEST_CASE("Test return types", "[js]") {
  using namespace Catch::Matchers;

  JsModule module(std::make_shared<JsContext>(), "script_2", kScript2);
  REQUIRE(4 == module.call<int>("add", 3, 1));
  REQUIRE(3.0 == module.call<double>("mul", 3, 1));
  REQUIRE("31" == module.call<std::string>("strcat", 3, 1));
  REQUIRE_THAT(
      module.call<std::vector<double>>("arrcat", 3, 1),
      Equals<double>({3.0, 1.0}));
  REQUIRE_THAT(
      module.call<std::vector<int>>("arrcat", 4, 5, 6), Equals<int>({4, 5, 6}));
}

TEST_CASE("Test globals", "[js]") {
  using namespace Catch::Matchers;

  auto js = std::make_shared<JsContext>();
  js->setGlobal("a_number", 3.2);
  js->setGlobal("a_bool", true);
  js->setGlobal("an_int", 7);
  js->setGlobal("a_string", "foobar");
  js->setGlobal("an_array", std::vector<std::string>{"hello", "joe"});
  js->setGlobal(
      "a_map",
      std::unordered_map<std::string, int>{
          {"hello", 1},
          {"bloe", 2},
      });
  REQUIRE(3.2 == js->getGlobal<double>("a_number"));
  REQUIRE(true == js->getGlobal<bool>("a_bool"));
  REQUIRE(7 == js->getGlobal<int>("an_int"));
  REQUIRE("foobar" == js->getGlobal<std::string>("a_string"));
  REQUIRE_THAT(
      js->getGlobal<std::vector<std::string>>("an_array"),
      Equals<std::string>({"hello", "joe"}));

  auto global_map =
      js->getGlobal<std::unordered_map<std::string, int>>("a_map");
  REQUIRE(2 == global_map.size());
  REQUIRE(1 == global_map.at("hello"));
  REQUIRE(2 == global_map.at("bloe"));

  JsModule module(js, "script_3", kScript3);
  REQUIRE(3.2 == module.call<double>("get_a_number"));
  REQUIRE(true == module.call<bool>("get_a_bool"));
  REQUIRE(7 == module.call<int>("get_an_int"));
  REQUIRE("foobar" == module.call<std::string>("get_a_string"));
  REQUIRE_THAT(
      module.call<std::vector<std::string>>("get_an_array"),
      Equals<std::string>({"hello", "joe"}));

  global_map = module.call<std::unordered_map<std::string, int>>("get_a_map");
  REQUIRE(2 == global_map.size());
  REQUIRE(1 == global_map.at("hello"));
  REQUIRE(2 == global_map.at("bloe"));
}

TEST_CASE("Test functions", "[js]") {
  auto js = std::make_shared<JsContext>();
  js->setGlobal(
      "fn1", make_function([](std::string a) { return format("__%1%__", a); }));
  js->setGlobal("fn2", make_function([](int a, int b) { return 2 * a + b; }));
  js->setGlobal("fn3", make_function([](int a, double b, bool c) {
                  return format("%1%:%2%:%3%", a, b, c);
                }));

  JsModule module(js, "script_4", kScript4);
  REQUIRE("__foo__" == module.call<std::string>("invoke1", "foo"));
  REQUIRE(5 == module.call<int>("invoke2", 2, 1));
  REQUIRE(9 == module.call<int>("invoke2", 4, 1));
  REQUIRE(11 == module.call<int>("invoke2", 3, 5));
  REQUIRE("3:4.72:0" == module.call<std::string>("invoke3", 3, 4.72, false));
}

}  // namespace tequila