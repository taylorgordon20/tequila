#pragma once

#include <boost/any.hpp>
#include <boost/container_hash/extensions.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_set>

#include "src/common/caches.hpp"
#include "src/common/concurrency.hpp"
#include "src/common/errors.hpp"
#include "src/common/functions.hpp"
#include "src/common/utils.hpp"

namespace tequila {

class Resources;
class ResourceDeps;

// Extracts a key tuple for Resources defined with non-const factories.
template <
    typename Resource,
    typename... Keys,
    typename Return,
    typename Class,
    typename... Args>
auto resourceHashTuple(
    Return (Class::*factory_fn)(ResourceDeps&, Args...),
    int seed,
    const Keys&... keys)
    -> std::tuple<std::type_index, int, std::decay_t<Args>...> {
  return std::tuple<std::type_index, int, std::decay_t<Args>...>(
      std::type_index(typeid(Resource)), seed, keys...);
}

// Extracts a key tuple for Resources defined with const factories.
template <
    typename Resource,
    typename... Keys,
    typename Return,
    typename Class,
    typename... Args>
auto resourceHashTuple(
    Return (Class::*factory_fn)(ResourceDeps&, Args...) const,
    int seed,
    const Keys&... keys)
    -> std::tuple<std::type_index, int, std::decay_t<Args>...> {
  return std::tuple<std::type_index, int, std::decay_t<Args>...>(
      std::type_index(typeid(Resource)), seed, keys...);
}

// Generates a 64-bit hash of a resource key tuple.
template <typename Resource, typename... Keys>
uint64_t resourceHash(const Keys&... keys) {
  auto lo_tuple =
      resourceHashTuple<Resource>(&Resource::operator(), 1269021407, keys...);
  auto hi_tuple =
      resourceHashTuple<Resource>(&Resource::operator(), 2139465699, keys...);
  uint32_t lo = boost::hash_value(lo_tuple);
  uint64_t hi = boost::hash_value(hi_tuple);
  return (hi << 32) + lo;
}

class ResourceGeneratorBase {
 public:
  virtual ~ResourceGeneratorBase() = default;

  // Returns key uniquely identifying this generator.
  virtual uint64_t key() = 0;

  // Returns the resource type of this generator.
  virtual const std::type_info& type() = 0;

  // Atomically removes the resource with the given key as a subscriber.
  virtual void unsubscribe(uint64_t resource_key) = 0;

  // Atomically adds the resource with the given key as a subscriber.
  virtual void subscribe(uint64_t resource_key) = 0;

  // Atomically returns the resource keys of the current subscribers.
  virtual std::shared_ptr<std::unordered_set<uint64_t>> subscribers() = 0;

  // Atomically returns the resource keys of the current dependencies.
  virtual std::shared_ptr<std::unordered_set<uint64_t>> dependencies() = 0;

  // Clears the resource version and returns the current subscribers.
  virtual void clear() = 0;

  // Returns true if the currently cached value is stale.
  virtual bool stale() = 0;
};

template <typename Resource>
using ResourceValue =
    typename decltype(make_function(&Resource::operator()))::result_type;

// Reduced interface exposed to resource factories allowing dependency injection
// of other resources. The dependencies are tracked to allow update propagation.
class ResourceDeps {
 public:
  ResourceDeps(Resources& resources, uint64_t resource_key)
      : resources_(resources), resource_key_(resource_key) {}

  template <typename Resource, typename... Keys>
  ResourceValue<Resource> get(const Keys&... keys);

  auto& deps() {
    return deps_;
  }

 private:
  using DepPtr = std::shared_ptr<ResourceGeneratorBase>;

  Resources& resources_;
  uint64_t resource_key_;
  std::unordered_map<uint64_t, DepPtr> deps_;
};

// The wrapper type stored in the resource cache that manages the life cycle
// of a resource as well as its subscribers and dependencies.
template <typename Resource>
class ResourceGenerator : public ResourceGeneratorBase {
  using Value = ResourceValue<Resource>;

 public:
  template <typename Function, typename... Keys>
  ResourceGenerator(Resources& resources, Function fn, const Keys&... keys)
      : resources_(resources),
        key_(resourceHash<Resource>(keys...)),
        version_(0),
        requested_version_(1),
        cached_subs_(std::make_shared<std::unordered_set<uint64_t>>()),
        cached_deps_(std::make_shared<std::unordered_set<uint64_t>>()) {
    generator_ = [this, fn = std::move(fn), keys = std::make_tuple(keys...)] {
      std::lock_guard<std::mutex> generator_lock(generator_mutex_);

      // Return if the current version is up to date.
      ENFORCE(version_ <= requested_version_);
      if (version_ == requested_version_) {
        return *get_ptr();
      }

      // Build a new version.
      version_ = requested_version_.load();
      ResourceDeps deps(resources_, key_);
      auto value = std::apply(
          [&](const auto&... keys) {
            return std::make_shared<Value>(fn(deps, keys...));
          },
          keys);

      // We need to make sure that the old value isn't destroyed under the
      // mutex otherwise we could deadlock.
      auto old_value = value_;

      // Update the value to the new version if its the latest and make sure
      // that dependency subscription lists match the latest version.
      if (version_ == requested_version_) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::atomic_store(&value_, value);
        auto& new_deps = deps.deps();
        for (const auto& pair : deps_) {
          if (!new_deps.count(pair.first)) {
            pair.second->unsubscribe(key_);
          }
        }
        deps_.swap(new_deps);
        cacheDeps();
      }

      return *value;
    };
  }

  ~ResourceGenerator() {
    std::lock_guard lock(mutex_);
    for (const auto& pair : deps_) {
      pair.second->unsubscribe(key_);
    }
  }

  // Returns a pointer to the currently cached value atomically.
  auto get_ptr() {
    return std::atomic_load(&value_);
  }

  // Returns the value atomically with caching.
  auto get() {
    if (auto ptr = get_ptr()) {
      if (!stale()) {
        return *ptr;
      }
    }
    return generator_();
  }

  // Returns key uniquely identifying this generator.
  uint64_t key() override {
    return key_;
  }

  // Returns the resource type of this generator.
  const std::type_info& type() override {
    return typeid(Resource);
  }

  // Atomically removes the resource with the given key as a subscriber.
  void unsubscribe(uint64_t resource_key) override {
    std::lock_guard lock(mutex_);
    if (subs_.count(resource_key)) {
      subs_.erase(resource_key);
      cacheSubs();
    }
  }

  // Atomically adds the resource with the given key as a subscriber.
  void subscribe(uint64_t resource_key) override {
    std::lock_guard lock(mutex_);
    if (!subs_.count(resource_key)) {
      subs_.insert(resource_key);
      cacheSubs();
    }
  }

  // Atomically returns the resource keys of the current subscribers.
  std::shared_ptr<std::unordered_set<uint64_t>> subscribers() override {
    return std::atomic_load(&cached_subs_);
  }

  // Atomically returns the resource keys of the current dependencies.
  std::shared_ptr<std::unordered_set<uint64_t>> dependencies() override {
    return std::atomic_load(&cached_deps_);
  }

  // Deletes the resource version and returns the current subscribers.
  void clear() override {
    std::lock_guard lock(mutex_);
    requested_version_++;
    subs_.clear();
  }

  // Deletes the resource version and returns the current subscribers.
  bool stale() override {
    return version_ < requested_version_;
  }

 private:
  void cacheDeps() {
    auto cached_deps = std::make_shared<std::unordered_set<uint64_t>>();
    for (const auto& pair : deps_) {
      cached_deps->insert(pair.first);
    }
    std::atomic_store(&cached_deps_, cached_deps);
  }

  void cacheSubs() {
    auto cached_subs = std::make_shared<std::unordered_set<uint64_t>>();
    *cached_subs = subs_;
    std::atomic_store(&cached_subs_, cached_subs);
  }

  Resources& resources_;
  uint64_t key_;
  std::atomic<uint64_t> version_;
  std::atomic<uint64_t> requested_version_;
  std::shared_ptr<std::unordered_set<uint64_t>> cached_subs_;
  std::shared_ptr<std::unordered_set<uint64_t>> cached_deps_;
  std::function<Value()> generator_;
  std::shared_ptr<Value> value_;
  std::unordered_map<uint64_t, std::shared_ptr<ResourceGeneratorBase>> deps_;
  std::unordered_set<uint64_t> subs_;
  std::mutex mutex_;
  std::mutex generator_mutex_;
};

// TODO: Implement cache eviction.
// TODO: Implement hash collision fallback.
class Resources {
 public:
  Resources() : mutex_(std::make_unique<std::shared_mutex>()) {}
  Resources(std::unordered_map<std::type_index, boost::any> overrides)
      : mutex_(std::make_unique<std::shared_mutex>()),
        overrides_(std::move(overrides)) {}

  template <typename Resource, typename... Keys>
  auto generator(const Keys&... keys) {
    auto cache_key = resourceHash<Resource>(keys...);

    // Return the value from cache immediately if available.
    {
      std::shared_lock lock(*mutex_);
      if (auto cached_generator = cachedGenerator<Resource>(cache_key)) {
        return cached_generator;
      }
    }

    // Create generator and cache it.
    {
      std::unique_lock exclusive_lock(*mutex_);
      if (auto cached_generator = cachedGenerator<Resource>(cache_key)) {
        return cached_generator;
      } else {
        auto generator = makeGenerator<Resource>(keys...);
        ENFORCE(cache_.emplace(cache_key, generator).second);
        return generator;
      }
    }
  }

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) {
    return generator<Resource>(keys...)->get();
  }

  template <typename Resource, typename... Keys>
  void invalidate(const Keys&... keys) {
    propagate(resourceHash<Resource>(keys...), [](auto generator) {
      generator->clear();
    });
  }

 private:
  template <typename Resource>
  auto cachedGenerator(uint64_t cache_key) {
    std::shared_ptr<ResourceGenerator<Resource>> ret;
    if (cache_.count(cache_key)) {
      auto generator = cache_.at(cache_key);
      ENFORCE(
          typeid(Resource) == generator->type(),
          concat(
              "Cache collision ",
              typeid(Resource).name(),
              " vs ",
              generator->type().name()));
      ret = std::static_pointer_cast<ResourceGenerator<Resource>>(generator);
    }
    return ret;
  }

  auto subscribers(uint64_t source_key) {
    std::unordered_map<uint64_t, std::shared_ptr<ResourceGeneratorBase>> ret;

    // We need to propagate changes in dependency order.
    std::unordered_set<uint64_t> done_keys;
    std::vector<uint64_t> key_stack{source_key};
    while (!key_stack.empty()) {
      auto key = key_stack.back();
      key_stack.pop_back();
      done_keys.insert(key);

      // Get this generator from cache if it exists.
      auto generator = [&] {
        std::shared_lock lock(*mutex_);
        if (cache_.count(key)) {
          return cache_.at(key);
        } else {
          return std::shared_ptr<ResourceGeneratorBase>();
        }
      }();

      // Invalidate the generator and add its subs to the stack.
      if (generator) {
        ret[key] = generator;
        for (auto sub : *generator->subscribers()) {
          if (!done_keys.count(sub)) {
            key_stack.push_back(sub);
          }
        }
      }
    }

    return ret;
  }

  template <typename Function>
  void propagate(uint64_t source_key, Function&& fn) {
    auto gens = subscribers(source_key);
    for (auto& pair : gens) {
      fn(pair.second);
    }
  }

  template <typename Resource, typename... Keys>
  auto makeGenerator(const Keys&... keys) {
    using Fun = decltype(make_function(&Resource::operator()));
    using Gen = ResourceGenerator<Resource>;
    auto it = overrides_.find(std::type_index(typeid(Resource)));
    if (it != overrides_.end()) {
      auto fun = boost::any_cast<Fun>(it->second);
      return std::make_shared<Gen>(*this, fun, keys...);
    } else {
      auto fun = [](auto&&... args) { return Resource()(args...); };
      return std::make_shared<Gen>(*this, fun, keys...);
    }
  }

  std::unique_ptr<std::shared_mutex> mutex_;
  std::unordered_map<std::type_index, boost::any> overrides_;
  std::unordered_map<uint64_t, std::shared_ptr<ResourceGeneratorBase>> cache_;
};

template <typename Resource, typename... Keys>
ResourceValue<Resource> ResourceDeps::get(const Keys&... keys) {
  try {
    auto generator = resources_.generator<Resource>(keys...);
    generator->subscribe(resource_key_);
    deps_.emplace(resourceHash<Resource>(keys...), generator);
    return generator->get();
  } catch (const std::exception& e) {
    LOG_ERROR(format(
        "Resource \"%1%\" exception: %2%", typeid(Resource).name(), e.what()));
    throw;
  } catch (...) {
    LOG_ERROR(format("Resource \"%1%\" exception.", typeid(Resource).name()));
    throw;
  }
}

class AsyncResources {
 public:
  AsyncResources(
      std::shared_ptr<Resources> resources,
      std::shared_ptr<QueueExecutor> executor)
      : resources_(std::move(resources)), executor_(std::move(executor)) {}

  std::shared_ptr<Resources> resources() {
    return resources_;
  }

  template <typename Resource, typename... Keys>
  auto get_opt(const Keys&... keys) {
    // If there is a cached value, return it immediately and kick off an
    // asynchronous update if the cached value is stale.
    boost::optional<decltype(resources()->get<Resource>(keys...))> ret;
    auto generator = resources()->generator<Resource>(keys...);
    if (auto value_ptr = generator->get_ptr()) {
      ret = *value_ptr;
    }
    if (generator->stale()) {
      get<Resource>(keys...);
    }
    return ret;
  }

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) {
    return schedule(
        concat("get", describe<Resource>(keys...)),
        [this](const auto&... keys) {
          return resources()->get<Resource>(keys...);
        },
        keys...);
  }

  template <typename Resource, typename... Keys>
  auto invalidate(const Keys&... keys) {
    // Invalidate the value asynchronously and return a future.
    return schedule(
        concat("invalidate", describe<Resource>(keys...)),
        [this](const auto&... keys) {
          return resources()->invalidate<Resource>(keys...);
        },
        keys...);
  }

 private:
  template <typename Resource, typename... Keys>
  auto describe(const Keys&... keys) {
    return format("<%1%>(%2%)", typeid(Resource).name(), join(", ", keys...));
  }

  template <typename Function, typename... Keys>
  auto schedule(const std::string& task, Function fn, const Keys&... keys) {
    return executor_->schedule(
        [task, fn = std::move(fn), keys = std::make_tuple(keys...)] {
          try {
            return std::apply(fn, keys);
          } catch (const std::exception& e) {
            LOG_ERROR(format(
                "Async resource error:  %1%. Task: %2%", e.what(), task));
            throw;
          }
        });
  }

  std::shared_ptr<Resources> resources_;
  std::shared_ptr<QueueExecutor> executor_;
};  // namespace tequila

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
  ResourcesBuilder& withSeed(ValueType&& value) {
    decltype(make_function(Resource())) fn =
        [value = std::forward<ValueType>(value)](
            ResourceDeps& resources, auto... args) {
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
struct SeedResource {
  Value operator()(ResourceDeps& deps) {
    throwError("Missing seed resource: %1%", typeid(Resource).name());
  }
};

template <typename Resource, typename... Keys>
class ResourceMutation {
 public:
  ResourceMutation(Resources& resources, const Keys&... keys)
      : value_(resources.get<Resource>(keys...)) {
    invalidator_ = [&resources, keys...] {
      resources.invalidate<Resource>(keys...);
    };
  }

  ResourceMutation(AsyncResources& resources, const Keys&... keys)
      : value_(resources.get<Resource>(keys...).get()) {
    invalidator_ = [&resources, keys...] {
      resources.invalidate<Resource>(keys...);
    };
  }

  ~ResourceMutation() {
    invalidator_();
  }

  auto operator-> () {
    return value_;
  }

 private:
  decltype(Resources().get<Resource>(std::declval<Keys>()...)) value_;
  std::function<void()> invalidator_;
};

}  // namespace tequila