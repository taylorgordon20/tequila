#pragma once

#include <ChakraCore.h>

#include <array>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/functions.hpp"
#include "src/common/strings.hpp"

namespace tequila {

#define JS_ENFORCE(js_call)                                                \
  do {                                                                     \
    auto status = (js_call);                                               \
    if (status == JsErrorScriptException) {                                \
      throwJsError();                                                      \
    } else if (status == JsErrorScriptCompile) {                           \
      throwJsError();                                                      \
    } else if (status != JsNoError) {                                      \
      throwError("JsError: '%1%' at %2%:%3%", status, __FILE__, __LINE__); \
    }                                                                      \
  } while (0)

class JsModules {
 public:
  JsModules() {
    JS_ENFORCE(JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime_));
    JS_ENFORCE(JsCreateContext(runtime_, &context_));
  }

  ~JsModules() noexcept {
    JsDisposeRuntime(runtime_);
  }

  int64_t load(const std::string& code) {
    ContextGuard ctx_guard(context_);

    static int64_t script_id_counter = 0;
    auto script_id = script_id_counter++;
    auto script_name = format("%1%.js", script_id);

    // Load the code into a JS value.
    JsValueRef script;
    JS_ENFORCE(JsCreateExternalArrayBuffer(
        (void*)code.c_str(), code.size(), nullptr, nullptr, &script));

    // Run the script and store its result into a module reference.
    JsSourceContext src_ctx = 0;
    JsValueRef src_url = value(script_name);
    JsValueRef module;
    JS_ENFORCE(
        JsRun(script, src_ctx, src_url, JsParseScriptAttributeNone, &module));

    JS_ENFORCE(JsAddRef(module, nullptr));
    modules_[script_id] = module;
    return script_id;
  }

  void drop(int64_t script_id) {
    ContextGuard ctx_guard(context_);
    auto module = modules_.at(script_id);
    modules_.erase(script_id);
    JsRelease(module, nullptr);
  }

  template <typename Return, typename... Args>
  Return call(int64_t script_id, const std::string& fn, Args&&... args) {
    ContextGuard ctx_guard(context_);
    auto& module = modules_.at(script_id);

    // Get a reference to the JS function.
    JsValueRef fn_ref = property(module, fn);

    // Prepare the function arguments.
    std::array<JsValueRef, 1 + sizeof...(Args)> fn_args;
    fn_args.at(0) = undefined();
    {
      size_t i = 1;
      ((void)(fn_args[i++] = value(std::forward<Args>(args))), ...);
    }

    // Invoke the function and return its return value.
    JsValueRef fn_ret;
    JS_ENFORCE(JsCallFunction(fn_ref, fn_args.data(), fn_args.size(), &fn_ret));
    return cast<Return>(fn_ret);
  }

  void delGlobal(const std::string& name) {
    ContextGuard ctx_guard(context_);
    JS_ENFORCE(JsDeleteProperty(global(), propertyId(name), true, nullptr));
  }

  template <typename Arg>
  void setGlobal(const std::string& name, Arg&& arg) {
    ContextGuard ctx_guard(context_);
    setProperty(global(), name, value(std::forward<Arg>(arg)));
  }

  template <typename Return>
  Return getGlobal(const std::string& name) {
    ContextGuard ctx_guard(context_);
    return cast<Return>(property(global(), name));
  }

 protected:
  class ContextGuard {
   public:
    ContextGuard(JsContextRef context) {
      ENFORCE(JsNoError == JsSetCurrentContext(context));
    }
    ~ContextGuard() noexcept {
      JsSetCurrentContext(JS_INVALID_REFERENCE);
    }
  };

  template <typename... Args, size_t... Is>
  auto argsTuple(JsValueRef* args, std::index_sequence<Is...>) {
    return std::make_tuple(cast<Args>(args[1 + Is])...);
  }

  // Simple value creation functions.
  JsValueRef undefined();
  JsValueRef global();
  JsValueRef value(double value);
  JsValueRef value(int value);
  JsValueRef value(bool value);
  JsValueRef value(const char* value);
  JsValueRef value(const std::string& value);

  // Object value creation functions.
  template <typename ValueType>
  JsValueRef value(const std::vector<ValueType>& value);

  template <typename ValueType>
  JsValueRef value(const std::unordered_map<std::string, ValueType>& value);

  // Function creation functions.
  template <typename Return, typename... Args>
  JsValueRef value(std::function<Return(Args...)> fn);

  // Object property functions.
  JsPropertyIdRef propertyId(const std::string& prop);
  JsValueRef property(JsValueRef obj, const std::string& prop);
  void setProperty(JsValueRef obj, const std::string& prop, JsValueRef value);

  // Array index functions.
  JsValueRef index(JsValueRef obj, int index);
  void setIndex(JsValueRef obj, int index, JsValueRef value);

  // Value dereferencing functions.
  template <typename ValueType>
  void castTo(JsValueRef value, ValueType& out);

  template <typename ValueType>
  void castTo(JsValueRef value, std::vector<ValueType>& out);

  template <typename ValueType>
  void castTo(
      JsValueRef value, std::unordered_map<std::string, ValueType>& out);

  template <typename ReturnType>
  ReturnType cast(JsValueRef value) {
    ReturnType ret;
    castTo(value, ret);
    return ret;
  }

  // Error handling.
  void throwJsError() {
    JsValueRef exception;
    JS_ENFORCE(JsGetAndClearException(&exception));
    throwError(
        "JsError: '%1%' Message: '%2%'",
        cast<std::string>(property(exception, "name")),
        cast<std::string>(property(exception, "message")));
  }

 private:
  JsRuntimeHandle runtime_;
  JsContextRef context_;
  std::unordered_map<int64_t, JsValueRef> modules_;
};

JsValueRef JsModules::undefined() {
  JsValueRef ret;
  JS_ENFORCE(JsGetUndefinedValue(&ret));
  return ret;
}

JsValueRef JsModules::global() {
  JsValueRef ret;
  JS_ENFORCE(JsGetGlobalObject(&ret));
  return ret;
}

JsValueRef JsModules::value(double value) {
  JsValueRef ret;
  JS_ENFORCE(JsDoubleToNumber(value, &ret));
  return ret;
}

JsValueRef JsModules::value(int value) {
  JsValueRef ret;
  JS_ENFORCE(JsIntToNumber(value, &ret));
  return ret;
}

JsValueRef JsModules::value(bool value) {
  JsValueRef ret;
  JS_ENFORCE(JsBoolToBoolean(value, &ret));
  return ret;
}

JsValueRef JsModules::value(const char* value) {
  return this->value(std::string(value));
}

JsValueRef JsModules::value(const std::string& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateString(value.c_str(), value.size(), &ret));
  return ret;
}

template <typename ValueType>
JsValueRef JsModules::value(const std::vector<ValueType>& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateArray(value.size(), &ret));
  for (int i = 0; i < value.size(); i += 1) {
    JS_ENFORCE(
        JsSetIndexedProperty(ret, this->value(i), this->value(value.at(i))));
  }
  return ret;
}

template <typename ValueType>
JsValueRef JsModules::value(
    const std::unordered_map<std::string, ValueType>& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateObject(&ret));
  for (const auto& pair : value) {
    setProperty(ret, pair.first, this->value(pair.second));
  }
  return ret;
}

template <typename Return, typename... Args>
JsValueRef JsModules::value(std::function<Return(Args...)> fn) {
  // Move the function onto the heap so that it can be managed by the JSRT.
  auto fn_ptr = new std::function<JsValueRef(JsValueRef*)>(
      [this, fn = std::move(fn)](JsValueRef* args) {
        auto tup = argsTuple<Args...>(args, std::index_sequence_for<Args...>());
        return value(std::apply(fn, tup));
      });

  // Adapts a JS function call to its corresponding std::function.
  auto adapter_fn = [](JsValueRef callee,
                       bool is_construct_call,
                       JsValueRef* arguments,
                       unsigned short arguments_count,
                       void* data) {
    ENFORCE(arguments_count == 1 + sizeof...(Args));
    return (*static_cast<decltype(fn_ptr)>(data))(arguments);
  };

  // Deletes the std::function for a garbage collected JS function.
  auto collect_fn = [](JsRef, void* data) {
    delete static_cast<decltype(fn_ptr)>(data);
  };

  JsValueRef ret;
  JS_ENFORCE(JsCreateFunction(adapter_fn, (void*)fn_ptr, &ret));
  JS_ENFORCE(JsSetObjectBeforeCollectCallback(ret, (void*)fn_ptr, collect_fn));
  return ret;
}

JsPropertyIdRef JsModules::propertyId(const std::string& property) {
  JsPropertyIdRef ret;
  JS_ENFORCE(JsCreatePropertyId(property.c_str(), property.size(), &ret));
  return ret;
}

JsValueRef JsModules::property(JsValueRef obj, const std::string& prop) {
  JsValueRef ret;
  JS_ENFORCE(JsGetProperty(obj, propertyId(prop), &ret));
  return ret;
}

void JsModules::setProperty(
    JsValueRef object, const std::string& prop, JsValueRef val) {
  JS_ENFORCE(JsSetProperty(object, propertyId(prop), val, true));
}

JsValueRef JsModules::index(JsValueRef object, int index) {
  JsValueRef ret;
  JS_ENFORCE(JsGetIndexedProperty(object, value(index), &ret));
  return ret;
}

void JsModules::setIndex(JsValueRef object, int index, JsValueRef val) {
  JS_ENFORCE(JsSetIndexedProperty(object, value(index), val));
}

template <typename... Args>
void call(int64_t script_id, const std::string& fn, Args&&... args) {
  auto& module = modules_.at(script_id);
  JS_ENFORCE(JsCallFunction(property(module, fn), nullptr, 0, nullptr));
}

template <>
void JsModules::castTo(JsValueRef value, std::string& out) {
  size_t length;
  JS_ENFORCE(JsCopyString(value, nullptr, 0, &length));

  out.resize(length);
  JS_ENFORCE(JsCopyString(value, out.data(), length, &length));
}

template <>
void JsModules::castTo(JsValueRef value, double& out) {
  JS_ENFORCE(JsNumberToDouble(value, &out));
}

template <>
void JsModules::castTo(JsValueRef value, int& out) {
  JS_ENFORCE(JsNumberToInt(value, &out));
}

template <>
void JsModules::castTo(JsValueRef value, bool& out) {
  JS_ENFORCE(JsBooleanToBool(value, &out));
}

template <typename ValueType>
void JsModules::castTo(JsValueRef value, std::vector<ValueType>& out) {
  int out_size = cast<int>(property(value, "length"));
  out.resize(out_size);
  for (int i = 0; i < out_size; i += 1) {
    castTo(index(value, i), out[i]);
  }
}

template <typename ValueType>
void JsModules::castTo(
    JsValueRef value, std::unordered_map<std::string, ValueType>& out) {
  JsValueRef property_names;
  JsGetOwnPropertyNames(value, &property_names);
  for (auto& prop : cast<std::vector<std::string>>(property_names)) {
    out[prop] = cast<ValueType>(property(value, prop));
  }
}

}  // namespace tequila