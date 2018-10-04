#pragma once

#define SOL_CHECK_ARGUMENTS 1
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1
#define SOL_SAFE_FUNCTION 1
#define SOL_SAFE_FUNCTION_CALLS 1
#include <sol.hpp>

#include <stdexcept>
#include <type_traits>
#include <typeinfo>

#include "src/common/strings.hpp"

namespace tequila {

using LuaError = std::runtime_error;

class LuaContext {
 public:
  LuaContext() {
    state_.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8);
    state()["__ffi"] = sol::new_table();
  }

  template <typename Global>
  void has(const std::string& name) {
    return state().get<Global>(name) != sol::lua_nil;
  }

  template <typename Global>
  Global get(const std::string& name) {
    return state().get<Global>(name);
  }

  template <typename Global>
  void set(const std::string& name, Global&& global) {
    state()["__ffi"][name] = typeid(std::decay_t<Global>).name();
    state()[name] = std::forward<Global>(global);
  }

  sol::state& state() {
    return state_;
  }

 private:
  sol::state state_;
};

class LuaModule {
 public:
  LuaModule(LuaContext& context, const std::string& code) {
    module_ = context.state().script(code);
    delete_ = [](LuaModule*) {};
  }

  ~LuaModule() {
    delete_(this);
  }

  auto& deleter() {
    return delete_;
  }

  auto& table() {
    return module_;
  }

  bool has(const std::string& name) {
    auto ret = module_.get<sol::optional<sol::object>>(name);
    return !!ret;
  }

  template <typename Value>
  Value get(const std::string& name) {
    return module_.get<Value>(name);
  }

  template <typename Value>
  void set(const std::string& name, Value&& value) {
    module_[name] = std::forward<Value>(value);
  }

  template <
      typename Ret,
      typename... Args,
      typename = std::enable_if_t<!std::is_void_v<Ret>>>
  Ret call(const std::string& fn, Args&&... args) {
    auto lua_fn = module_.get<sol::function>(fn);
    auto result = lua_fn.call(module_, std::forward<Args>(args)...);
    if (!result.valid()) {
      sol::error error = result;
      throw LuaError(format("Lua Error: %1%", error.what()));
    }
    return result;
  }

  template <
      typename Ret,
      typename... Args,
      typename = std::enable_if_t<std::is_void_v<Ret>>>
  void call(const std::string& fn, Args&&... args) {
    auto lua_fn = module_.get<sol::function>(fn);
    auto result = lua_fn.call(module_, std::forward<Args>(args)...);
    if (!result.valid()) {
      sol::error error = result;
      throw LuaError(format("Lua Error: %1%", error.what()));
    }
  }

 private:
  sol::table module_;
  std::function<void(LuaModule*)> delete_;
};

}  // namespace tequila