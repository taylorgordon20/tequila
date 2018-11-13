#include "src/common/traces.hpp"

#include <chrono>
#include <string>
#include <vector>

#include "src/common/errors.hpp"

namespace tequila {

thread_local std::vector<Trace*> thread_traces;

// static
void Trace::push(Trace* trace) {
  thread_traces.push_back(trace);
}

// static
void Trace::pop() {
  ENFORCE(thread_traces.size());
  thread_traces.pop_back();
}

// static
void Trace::tag(std::string key) {
  if (thread_traces.size()) {
    thread_traces.back()->addTag(std::move(key));
  }
}

}  // namespace tequila