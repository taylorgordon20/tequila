#pragma once

#include <codecvt>
#include <iostream>
#include <locale>
#include <memory>
#include <vector>

#include "src/common/registry.hpp"

namespace tequila {

class Console {
 public:
  Console(std::ostream& out)
      : fn_([](std::string) {}), out_(out), out_pos_(out_.tellp()) {
    history_.push_back(U"");
    display();
  }

  template <typename Function>
  void onLine(Function&& fn) {
    fn_ = std::move(fn);
  }

  std::string getLine() {
#if _MSC_VER >= 1900
    std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> converter;
    auto s_ptr = reinterpret_cast<const int32_t*>(history_.back().data());
    return converter.to_bytes(s_ptr, s_ptr + history_.back().size());
#else
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.to_bytes(history_.back());
#endif
  }

  void display() {
    out_.seekp(out_pos_);
    out_ << "\r>> " << getLine();
  }

  void update() {
    auto line = getLine();
    out_ << std::endl;
    out_pos_ = out_.tellp();
    fn_(std::move(line));
    history_.push_back(U"");
  }

  void process(char32_t codepoint) {
    if (codepoint == static_cast<unsigned int>('\b')) {
      history_.back().pop_back();
    } else if (codepoint == static_cast<unsigned int>('\n')) {
      update();
    } else {
      history_.back() += codepoint;
    }
    display();
  }

 private:
  std::function<void(std::string)> fn_;
  std::ostream& out_;
  std::streampos out_pos_;
  std::vector<std::u32string> history_;
};

template <>
std::shared_ptr<Console> gen(const Registry& registry) {
  return std::make_shared<Console>(std::cout);
}

}  // namespace tequila