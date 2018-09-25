#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "src/common/lua.hpp"
#include "src/common/strings.hpp"

namespace tequila {

constexpr auto kScript1 = R"lua(
string = require("string")

local module = {}

function module:foo()
  return "foo_string"
end

function module:bar(a_double, an_int, a_string)
  return string.format("D(%.2f), I(%d), S(%s)", a_double, an_int, a_string)
end

function module:join(numbers, separator)
  return table.concat(numbers, separator or ",")
end

function module:sum(numbers)
  local ret = 0
  for i, num in ipairs(numbers) do
    ret = ret + num
  end
  return ret
end

function module:glob()
  return hickory .. ":" .. sticks["b"] .. ":" .. sticks["a"]
end

function module:apply1(cb)
  return cb(3, 9.1, "joe")
end

function module:apply2(cb)
  return cb({1, 2, 3, 4, 5})
end

return module
)lua";

TEST_CASE("Test basic usage", "[lua]") {
  LuaContext lua;

  // Load a module and call some of its functions.
  LuaModule m1(lua, kScript1);
  REQUIRE(m1.has("foo"));
  REQUIRE_FALSE(m1.has("foop"));
  REQUIRE("foo_string" == m1.call<std::string>("foo"));
  REQUIRE(
      "D(3.23), I(42), S(barbar)" ==
      m1.call<std::string>("bar", 3.23, 42, "barbar"));
  REQUIRE("1,2,3" == m1.call<std::string>("join", std::vector<int>{1, 2, 3}));
  REQUIRE(6 == m1.call<int>("sum", std::vector<int>{1, 2, 3}));

  // Define some global values.
  lua.set("hickory", 1234);
  lua.set("sticks", std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});
  REQUIRE("1234:2:1" == m1.call<std::string>("glob"));

  // Function callbacks.
  auto cb1 = [](int x, float y, const std::string& z) {
    return format("%1%:%2%:%3%", x, y, z);
  };
  auto cb2 = [&](const sol::table& numbers) {
    return m1.call<std::string>("join", numbers, ":");
  };
  REQUIRE("3:9.1:joe" == m1.call<std::string>("apply1", sol::as_function(cb1)));
  REQUIRE("1:2:3:4:5" == m1.call<std::string>("apply2", sol::as_function(cb2)));
}

}  // namespace tequila