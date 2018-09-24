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

// Exception type propagated on any JSRT error.
struct JsException : public std::runtime_error {
  JsErrorCode error_code;
  JsException(const std::string& what, JsErrorCode error_code)
      : std::runtime_error(what), error_code(error_code) {}
};

#define JS_ENFORCE(js_call)                                                    \
  do {                                                                         \
    auto error_code = (js_call);                                               \
    if (error_code != JsNoError) {                                             \
      throw JsException(                                                       \
          format("JsError: '%1%' at %2%:%3%", error_code, __FILE__, __LINE__), \
          error_code);                                                         \
    }                                                                          \
  } while (0)

// Maintains the state for an embedded JavaScript instance.
class JsContext {
 public:
  JsContext() {
    JS_ENFORCE(JsCreateRuntime(JsRuntimeAttributeNone, nullptr, &runtime_));
    JS_ENFORCE(JsCreateContext(runtime_, &context_));
  }

  ~JsContext() noexcept {
    JsDisposeRuntime(runtime_);
  }

  template <typename FunctionType>
  auto run(FunctionType&& fn) {
    ContextGuard guard(context_);
    try {
      return fn();
    } catch (const JsException& e) {
      if (e.error_code == JsErrorScriptException ||
          e.error_code == JsErrorScriptCompile) {
        JsValueRef exception;
        JS_ENFORCE(JsGetAndClearException(&exception));
        throwError(
            "%1%\nError name: '%2%', message: '%3%'",
            e.what(),
            cast<std::string>(property(exception, "name")),
            cast<std::string>(property(exception, "message")));
      }
      throw;
    }
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

 private:
  class ContextGuard {
   public:
    ContextGuard(JsContextRef context) {
      JS_ENFORCE(JsSetCurrentContext(context));
    }
    ~ContextGuard() noexcept {
      JsSetCurrentContext(JS_INVALID_REFERENCE);
    }
  };

  template <typename... Args, size_t... Is>
  auto argsTuple(JsValueRef* args, std::index_sequence<Is...>) {
    return std::make_tuple(cast<Args>(args[1 + Is])...);
  }

  template <typename Return, typename... Args>
  auto applyFunc(
      const std::function<Return(Args...)>& fn, std::tuple<Args...>&& tup) {
    return value(std::apply(fn, std::move(tup)));
  }

  template <
      typename Return,
      typename... Args,
      typename = std::enable_if_t<std::is_void_v<Return>>>
  auto applyFunc(
      const std::function<void(Args...)>& fn, std::tuple<Args...>&& tup) {
    std::apply(fn, std::move(tup));
    return undefined();
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

  template <>
  void JsContext::cast<void>(JsValueRef value) {}

 private:
  JsRuntimeHandle runtime_;
  JsContextRef context_;

  friend class JsModule;
};

// Loads and maintains a reference to a JavaScript module object.
class JsModule {
 public:
  JsModule(
      std::shared_ptr<JsContext> context,
      const std::string& name,
      const std::string& code)
      : context_(std::move(context)) {
    ENFORCE(context_);
    context_->run([&] {
      // Load the code into a JS value.
      JsValueRef script;
      JS_ENFORCE(JsCreateExternalArrayBuffer(
          (void*)code.c_str(), code.size(), nullptr, nullptr, &script));

      // Run the script and store its result into a module reference.
      JsValueRef url = context_->value(format("%1%.js", name));
      JS_ENFORCE(JsRun(script, 0, url, JsParseScriptAttributeNone, &module_));

      // The module is managed externally so add reference count.
      JS_ENFORCE(JsAddRef(module_, nullptr));
    });
  }

  ~JsModule() noexcept {
    context_->run([&] { JsRelease(module_, nullptr); });
  }

  JsModule(const JsModule& other)
      : context_(other.context_), module_(other.module_) {
    context_->run([&] { JsAddRef(module_, nullptr); });
  }

  JsModule(JsModule&&) = delete;
  JsModule& operator=(const JsModule& other) = delete;
  JsModule& operator=(JsModule&&) = delete;

  template <typename Return, typename... Args>
  Return call(const std::string& fn_name, Args&&... args) {
    return context_->run([&] {
      // Get a reference to the JS function.
      JsValueRef fn = context_->property(module_, fn_name);

      // Prepare the function arguments.
      std::array<JsValueRef, 1 + sizeof...(Args)> fn_args;
      fn_args.at(0) = context_->undefined();
      {
        size_t i = 1;
        ((void)(fn_args[i++] = context_->value(std::forward<Args>(args))), ...);
      }

      // Invoke the function and return its return value.
      JsValueRef ret;
      JS_ENFORCE(JsCallFunction(fn, fn_args.data(), fn_args.size(), &ret));
      return context_->cast<Return>(ret);
    });
  }

  bool has(const std::string& property) {
    return context_->run([&] {
      bool ret;
      JS_ENFORCE(JsHasProperty(module_, context_->propertyId(property), &ret));
      return ret;
    });
  }

  template <typename Return>
  Return get(const std::string& name) {
    return context_->run([&] {
      JsValueRef ret;
      JS_ENFORCE(JsGetProperty(module_, context_->propertyId(property), &ret));
      return context_->cast<Return>(ret);
    });
  }

 private:
  std::shared_ptr<JsContext> context_;
  JsValueRef module_;
};

JsValueRef JsContext::undefined() {
  JsValueRef ret;
  JS_ENFORCE(JsGetUndefinedValue(&ret));
  return ret;
}

JsValueRef JsContext::global() {
  JsValueRef ret;
  JS_ENFORCE(JsGetGlobalObject(&ret));
  return ret;
}

JsValueRef JsContext::value(double value) {
  JsValueRef ret;
  JS_ENFORCE(JsDoubleToNumber(value, &ret));
  return ret;
}

JsValueRef JsContext::value(int value) {
  JsValueRef ret;
  JS_ENFORCE(JsIntToNumber(value, &ret));
  return ret;
}

JsValueRef JsContext::value(bool value) {
  JsValueRef ret;
  JS_ENFORCE(JsBoolToBoolean(value, &ret));
  return ret;
}

JsValueRef JsContext::value(const char* value) {
  return this->value(std::string(value));
}

JsValueRef JsContext::value(const std::string& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateString(value.c_str(), value.size(), &ret));
  return ret;
}

template <typename ValueType>
JsValueRef JsContext::value(const std::vector<ValueType>& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateArray(value.size(), &ret));
  for (int i = 0; i < value.size(); i += 1) {
    JS_ENFORCE(
        JsSetIndexedProperty(ret, this->value(i), this->value(value.at(i))));
  }
  return ret;
}

template <typename ValueType>
JsValueRef JsContext::value(
    const std::unordered_map<std::string, ValueType>& value) {
  JsValueRef ret;
  JS_ENFORCE(JsCreateObject(&ret));
  for (const auto& pair : value) {
    setProperty(ret, pair.first, this->value(pair.second));
  }
  return ret;
}

template <typename Return, typename... Args>
JsValueRef JsContext::value(std::function<Return(Args...)> fn) {
  // Move the function onto the heap so that it can be managed by the JSRT.
  auto fn_ptr = new std::function<JsValueRef(JsValueRef*)>(
      [this, fn = std::move(fn)](JsValueRef* args) {
        auto tup = argsTuple<Args...>(args, std::index_sequence_for<Args...>());
        return applyFunc<Return>(fn, std::move(tup));
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

JsPropertyIdRef JsContext::propertyId(const std::string& property) {
  JsPropertyIdRef ret;
  JS_ENFORCE(JsCreatePropertyId(property.c_str(), property.size(), &ret));
  return ret;
}

JsValueRef JsContext::property(JsValueRef obj, const std::string& prop) {
  JsValueRef ret;
  JS_ENFORCE(JsGetProperty(obj, propertyId(prop), &ret));
  return ret;
}

void JsContext::setProperty(
    JsValueRef object, const std::string& prop, JsValueRef val) {
  JS_ENFORCE(JsSetProperty(object, propertyId(prop), val, true));
}

JsValueRef JsContext::index(JsValueRef object, int index) {
  JsValueRef ret;
  JS_ENFORCE(JsGetIndexedProperty(object, value(index), &ret));
  return ret;
}

void JsContext::setIndex(JsValueRef object, int index, JsValueRef val) {
  JS_ENFORCE(JsSetIndexedProperty(object, value(index), val));
}

template <typename... Args>
void call(int64_t script_id, const std::string& fn, Args&&... args) {
  auto& module = modules_.at(script_id);
  JS_ENFORCE(JsCallFunction(property(module, fn), nullptr, 0, nullptr));
}

template <>
void JsContext::castTo(JsValueRef value, std::string& out) {
  size_t length;
  JS_ENFORCE(JsCopyString(value, nullptr, 0, &length));

  out.resize(length);
  JS_ENFORCE(JsCopyString(value, out.data(), length, &length));
}

template <>
void JsContext::castTo(JsValueRef value, double& out) {
  JS_ENFORCE(JsNumberToDouble(value, &out));
}

template <>
void JsContext::castTo(JsValueRef value, int& out) {
  JS_ENFORCE(JsNumberToInt(value, &out));
}

template <>
void JsContext::castTo(JsValueRef value, bool& out) {
  JS_ENFORCE(JsBooleanToBool(value, &out));
}

template <typename ValueType>
void JsContext::castTo(JsValueRef value, std::vector<ValueType>& out) {
  int out_size = cast<int>(property(value, "length"));
  out.resize(out_size);
  for (int i = 0; i < out_size; i += 1) {
    castTo(index(value, i), out[i]);
  }
}

template <typename ValueType>
void JsContext::castTo(
    JsValueRef value, std::unordered_map<std::string, ValueType>& out) {
  JsValueRef property_names;
  JsGetOwnPropertyNames(value, &property_names);
  for (auto& prop : cast<std::vector<std::string>>(property_names)) {
    out[prop] = cast<ValueType>(property(value, prop));
  }
}

}  // namespace tequila