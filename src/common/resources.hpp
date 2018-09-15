#pragma once

#include <boost/any.hpp>
#include <boost/container_hash/extensions.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_set>

#include "src/common/caches.hpp"
#include "src/common/errors.hpp"
#include "src/common/functions.hpp"

namespace tequila {

class Resources;

// TODO: Implement eviction.
// TODO: Implement hash collision fallback.
class ResourceCache {
 public:
  class Insertion {
   public:
    Insertion(ResourceCache& cache, uint64_t key) : cache_(cache), key_(key) {
      cache_.insertion_stack_.push_back(this);
    }
    ~Insertion() noexcept {
      assert(cache_.insertion_stack_.size());
      assert(cache_.insertion_stack_.back() == this);
      cache_.keys_.insert(key_);
      cache_.vals_[key_] = std::move(val_);
      for (auto dep : deps_) {
        if (cache_.keys_.count(dep)) {
          cache_.subs_[dep].insert(key_);
        }
      }
      cache_.deps_[key_] = std::move(deps_);
      cache_.insertion_stack_.pop_back();
    }

    void set(boost::any value) {
      val_ = std::move(value);
    }

    void addDep(uint64_t dep) {
      deps_.insert(dep);
    }

   private:
    ResourceCache& cache_;
    uint64_t key_;
    boost::any val_;
    std::unordered_set<uint64_t> deps_;
  };

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) {
    using Value = decltype(Resource()(std::declval<Resources>(), keys...));
    boost::optional<Value> ret;
    auto key = getKey<Resource>(keys...);
    if (vals_.count(key)) {
      ret = boost::any_cast<Value>(vals_.at(key));
    }
    if (insertion_stack_.size()) {
      insertion_stack_.back()->addDep(key);
    }
    return ret;
  }

  template <typename Resource, typename... Keys>
  auto reserve(const Keys&... keys) {
    auto key = getKey<Resource>(keys...);
    ENFORCE(!keys_.count(key), "Hash collision! Everything broken...");
    return Insertion(*this, key);
  }

  template <typename Resource, typename... Keys>
  void invalidate(const Keys&... keys) {
    // Remove all transitive dependencies from cache.
    std::vector<uint64_t> stack{getKey<Resource>(keys...)};
    std::unordered_set<uint64_t> visits{stack.back()};
    while (stack.size()) {
      auto key = stack.back();
      stack.pop_back();
      for (auto sub : subs_[key]) {
        if (visits.insert(sub).second) {
          stack.push_back(sub);
        }
      }
      subs_.erase(key);
      deps_.erase(key);
      keys_.erase(key);
      vals_.erase(key);
    }
  }

 private:
  template <typename Resource, typename... Keys>
  auto getKey(const Keys&... keys) const {
    constexpr auto s1 = 1269021407;
    constexpr auto s2 = 2139465699;
    auto ti = std::type_index(typeid(Resource));
    uint64_t lo = boost::hash_value(std::make_tuple(s1, ti, keys...));
    uint64_t hi = boost::hash_value(std::make_tuple(s2, ti, keys...));
    return (hi << 32) + lo;
  }

  std::unordered_set<uint64_t> keys_;
  std::unordered_map<uint64_t, boost::any> vals_;
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> deps_;
  std::unordered_map<uint64_t, std::unordered_set<uint64_t>> subs_;
  std::vector<Insertion*> insertion_stack_;
};

class Resources {
 public:
  Resources() = default;
  Resources(std::unordered_map<std::type_index, boost::any> overrides)
      : overrides_(std::move(overrides)) {}

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) const {
    // If the value is in cache, just return it.
    if (auto value = cache_.get<Resource>(keys...)) {
      return *value;
    }

    // Generate the resource value and cache it. Any other resource that is
    // served from cache during generation will be marked as a dependency.
    auto cache_insertion = cache_.reserve<Resource>(keys...);
    auto value = gen<Resource>(keys...);
    cache_insertion.set(value);
    return value;
  }

  template <typename Resource, typename... Keys>
  void invalidate(const Keys&... keys) {
    cache_.invalidate<Resource>(keys...);
  }

 private:
  template <typename Resource, typename... Keys>
  auto gen(const Keys&... keys) const {
    // Generate the value using the override function if it is available,
    // otherwise default to using the static resource factory function.
    auto resource_key = std::type_index(typeid(Resource));
    if (overrides_.count(resource_key)) {
      using Fun = decltype(make_function(&Resource::operator()));
      const auto& any_fn = overrides_.at(resource_key);
      return boost::any_cast<Fun>(any_fn)(*this, keys...);
    } else {
      return Resource()(*this, keys...);
    }
  }

  std::unordered_map<std::type_index, boost::any> overrides_;
  mutable ResourceCache cache_;
};

class ResourcesBuilder {
 public:
  template <typename Resource, typename FunctionType>
  ResourcesBuilder& withOverride(FunctionType fn) {
    static_assert(
        std::is_same_v<
            decltype(make_function(&Resource::operator())),
            decltype(make_function(fn))>,
        "Function overrides must match the Resource operator() signature.");
    overrides_[std::type_index(typeid(Resource))] = make_function(fn);
    return *this;
  }

  template <typename Resource, typename ValueType>
  ResourcesBuilder& withSingleton(ValueType&& value) {
    decltype(make_function(Resource())) fn =
        [value = std::forward<ValueType>(value)](
            const Resources& resources, auto... args) {
          decltype(Resource()(resources, args...)) ret = value;
          return ret;
        };
    overrides_[std::type_index(typeid(Resource))] = fn;
    return *this;
  }

  Resources build() {
    return Resources(std::move(overrides_));
  }

 private:
  std::unordered_map<std::type_index, boost::any> overrides_;
};

template <typename Resource, typename Value>
struct SingletonResource {
  Value operator()(const Resources& resources) {
    throwError("Missing singleton resource: %1%", typeid(Resource).name());
  }
};

}  // namespace tequila
