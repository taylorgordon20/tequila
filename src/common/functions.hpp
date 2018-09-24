#pragma once

#include <functional>
#include <utility>

namespace tequila {

// Plain function pointers
template <typename... Args, typename Return>
auto make_function(Return (*fn)(Args...)) -> std::function<Return(Args...)> {
  return {fn};
}

// Non-const member function pointers
template <typename... Args, typename Return, typename Class>
auto make_function(Return (Class::*fn)(Args...))
    -> std::function<Return(Args...)> {
  return {fn};
}

// Const member function pointers
template <typename... Args, typename Return, typename Class>
auto make_function(Return (Class::*fn)(Args...) const)
    -> std::function<Return(Args...)> {
  return {fn};
}

// Functionoids (e.g. lambdas) with arguments explicitly specified.
template <typename First, typename... Args, typename Fn>
auto make_function(Fn fn) -> std::function<decltype(
    fn(std::declval<First>(), std::declval<Args>()...))(First, Args...)> {
  return {std::move(fn)};
}

// Functionoids (e.g. lambdas) with arguments deduced.
template <typename Fn>
auto make_function(Fn fn) -> decltype(make_function(&Fn::operator())) {
  return {std::move(fn)};
}

}  // namespace tequila