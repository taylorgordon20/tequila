#pragma once

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace tequila {

template <typename StringType, typename... Args>
inline auto format(StringType fmt, Args&&... args) {
  return (boost::format(fmt) % ... % std::forward<Args>(args)).str();
}

template <typename T>
inline auto concat(T&& t) {
  return format("%1%", std::forward<T>(t));
}

template <typename T1, typename T2, typename... Args>
inline auto concat(T1&& t1, T2&& t2, Args&&... args) {
  return concat(
      format("%1%%2%", std::forward<T1>(t1), std::forward<T2>(t2)),
      std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
inline auto to(Args&&... args) {
  return boost::lexical_cast<Ret>(concat(std::forward<Args>(args)...));
}

template <typename Separator>
inline std::string join(const Separator& sep) {
  return "";
}

template <typename Separator, typename T>
inline std::string join(const Separator& sep, T&& t) {
  return concat(std::forward<T>(t));
}

template <typename Separator, typename T1, typename T2, typename... Args>
inline std::string join(
    const Separator& sep, T1&& t1, T2&& t2, Args&&... args) {
  return join(
      sep,
      concat(std::forward<T1>(t1), sep, std::forward<T2>(t2)),
      std::forward<Args>(args)...);
}

template <typename Separator, typename Range>
inline auto joinRange(Separator&& separator, Range&& range) {
  std::stringstream ss;
  for (auto iter = range.begin(); iter != range.end();) {
    ss << *iter;
    if (++iter != range.end()) {
      ss << separator;
    }
  }
  return ss.str();
}

}  // namespace tequila