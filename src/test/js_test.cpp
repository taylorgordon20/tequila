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
  JsModules js;

  auto script_1 = js.load(kScript1);
  REQUIRE("foo_string" == js.call<std::string>(script_1, "foo"));
  REQUIRE("bar_string:6" == js.call<std::string>(script_1, "bar", 3.0));
  REQUIRE(
      "two_args:9dogs" ==
      js.call<std::string>(script_1, "two_args", 3.0, "dogs"));
  REQUIRE(
      "four_args:-1:true:lies:42" ==
      js.call<std::string>(script_1, "four_args", -1.0, true, "lies", 42));
  REQUIRE(
      "123456" ==
      js.call<std::string>(
          script_1, "array_args", std::vector<double>{1, 2, 3, 4, 5, 6}));
}

TEST_CASE("Test return types", "[js]") {
  using namespace Catch::Matchers;

  JsModules js;

  auto script_2 = js.load(kScript2);
  REQUIRE(4 == js.call<int>(script_2, "add", 3, 1));
  REQUIRE(3.0 == js.call<double>(script_2, "mul", 3, 1));
  REQUIRE("31" == js.call<std::string>(script_2, "strcat", 3, 1));
  REQUIRE_THAT(
      js.call<std::vector<double>>(script_2, "arrcat", 3, 1),
      Equals<double>({3.0, 1.0}));
  REQUIRE_THAT(
      js.call<std::vector<int>>(script_2, "arrcat", 4, 5, 6),
      Equals<int>({4, 5, 6}));
}

TEST_CASE("Test globals", "[js]") {
  using namespace Catch::Matchers;

  JsModules js;
  js.setGlobal("a_number", 3.2);
  js.setGlobal("a_bool", true);
  js.setGlobal("an_int", 7);
  js.setGlobal("a_string", "foobar");
  js.setGlobal("an_array", std::vector<std::string>{"hello", "joe"});
  js.setGlobal(
      "a_map",
      std::unordered_map<std::string, int>{
          {"hello", 1},
          {"bloe", 2},
      });
  REQUIRE(3.2 == js.getGlobal<double>("a_number"));
  REQUIRE(true == js.getGlobal<bool>("a_bool"));
  REQUIRE(7 == js.getGlobal<int>("an_int"));
  REQUIRE("foobar" == js.getGlobal<std::string>("a_string"));
  REQUIRE_THAT(
      js.getGlobal<std::vector<std::string>>("an_array"),
      Equals<std::string>({"hello", "joe"}));

  auto global_map = js.getGlobal<std::unordered_map<std::string, int>>("a_map");
  REQUIRE(2 == global_map.size());
  REQUIRE(1 == global_map.at("hello"));
  REQUIRE(2 == global_map.at("bloe"));

  auto script_3 = js.load(kScript3);
  REQUIRE(3.2 == js.call<double>(script_3, "get_a_number"));
  REQUIRE(true == js.call<bool>(script_3, "get_a_bool"));
  REQUIRE(7 == js.call<int>(script_3, "get_an_int"));
  REQUIRE("foobar" == js.call<std::string>(script_3, "get_a_string"));
  REQUIRE_THAT(
      js.call<std::vector<std::string>>(script_3, "get_an_array"),
      Equals<std::string>({"hello", "joe"}));
  global_map.clear();
  global_map =
      js.call<std::unordered_map<std::string, int>>(script_3, "get_a_map");
  REQUIRE(2 == global_map.size());
  REQUIRE(1 == global_map.at("hello"));
  REQUIRE(2 == global_map.at("bloe"));
}

TEST_CASE("Test functions", "[js]") {
  JsModules js;

  js.setGlobal(
      "fn1", make_function([](std::string a) { return format("__%1%__", a); }));
  js.setGlobal("fn2", make_function([](int a, int b) { return 2 * a + b; }));
  js.setGlobal("fn3", make_function([](int a, double b, bool c) {
                 return format("%1%:%2%:%3%", a, b, c);
               }));

  auto script_4 = js.load(kScript4);
  REQUIRE("__foo__" == js.call<std::string>(script_4, "invoke1", "foo"));
  REQUIRE(5 == js.call<int>(script_4, "invoke2", 2, 1));
  REQUIRE(9 == js.call<int>(script_4, "invoke2", 4, 1));
  REQUIRE(11 == js.call<int>(script_4, "invoke2", 3, 5));
  REQUIRE(
      "3:4.72:0" == js.call<std::string>(script_4, "invoke3", 3, 4.72, false));
}

}  // namespace tequila