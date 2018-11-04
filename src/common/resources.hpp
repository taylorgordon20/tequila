#pragma once

#include <boost/any.hpp>
#include <boost/container_hash/extensions.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <mutex>
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
    typename... Args>
auto resourceTuple(
    Return (Resource::*factory_fn)(const ResourceDeps&, Args...),
    const Keys&... keys) -> std::tuple<std::type_index, std::decay_t<Args>...> {
  return std::tuple<std::type_index, std::decay_t<Args>...>(
      std::type_index(typeid(Resource)), keys...);
}

// Extracts a key tuple for Resources defined with const factories.
template <
    typename Resource,
    typename... Keys,
    typename Return,
    typename... Args>
auto resourceTuple(
    Return (Resource::*factory_fn)(const ResourceDeps&, Args...) const,
    const Keys&... keys) -> std::tuple<std::type_index, std::decay_t<Args>...> {
  return std::tuple<std::type_index, std::decay_t<Args>...>(
      std::type_index(typeid(Resource)), keys...);
}

// Generates a 64-bit hash of a resource key tuple.
template <typename Resource, typename... Keys>
auto resourceHash(const Keys&... keys) {
  auto key_tuple = resourceTuple<Resource>(&Resource::operator(), keys...);
  uint64_t lo = boost::hash_value(std::make_pair(1269021407ull, key_tuple));
  uint64_t hi = boost::hash_value(std::make_pair(2139465699ull, key_tuple));
  return (hi << 32) + lo;
}

class ResourceGeneratorBase {
 public:
  // Returns key uniquely identifying this generator.
  virtual uint64_t key() = 0;

  // Atomically removes the resource with the given key as a subscriber.
  virtual void unsubscribe(uint64_t resource_key) = 0;

  // Atomically adds the resource with the given key as a subscriber.
  virtual void subscribe(uint64_t resource_key) = 0;

  // Atomically returns the resource keys of the current subscribers.
  virtual std::unordered_set<uint64_t> subscribers() = 0;

  // Atomically returns the resource keys of the current dependencies.
  virtual std::unordered_set<uint64_t> dependencies() = 0;

  // Builds the resource version and returns the current subscribers.
  virtual void build() = 0;

  // Clears the resource version and returns the current subscribers.
  virtual void clear() = 0;
};

// Reduced interface exposed to resource factories allowing dependency injection
// of other resources. The dependencies are tracked to allow update propagation.
class ResourceDeps {
 public:
  ResourceDeps(Resources& resources, uint64_t resource_key)
      : resources_(resources), resource_key_(resource_key) {}

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) const {
    auto generator = resources_.generator<Resource>(keys...);
    generator->subscribe(resource_key_);
    deps_.emplace(resourceHash<Resource>(keys...), generator);
    return generator->get();
  }

  auto& deps() {
    return deps_;
  }

 private:
  using DepPtr = std::shared_ptr<ResourceGeneratorBase>;

  Resources& resources_;
  uint64_t resource_key_;
  mutable std::unordered_map<uint64_t, DepPtr> deps_;
};

// The wrapper type stored in the resource cache that manages the life cycle
// of a resource as well as its subscribers and dependencies.
template <typename Resource, typename... Keys>
class ResourceGenerator : public ResourceGeneratorBase {
  using Value = decltype(
      Resource()(std::declval<ResourceDeps>(), std::declval<Keys>()...));

 public:
  template <typename Function, typename... Keys>
  ResourceGenerator(Resources& resources, Function&& fn, const Keys&... keys)
      : key_(resourceHash<Resource>(keys...)), version_(0) {
    generator_ =
        [&, fn = std::forward<Function>(fn), keys = std::make_tuple(keys...)] {
          uint64_t version = ++version_;

          // Build a new value version.
          ResourceDeps deps(resources, key_);
          auto value = std::apply(
              [&](const auto&... keys) {
                return std::make_shared<Value>(fn(deps, keys...));
              },
              keys);

          // Update the value to the new version if its the latest and make sure
          // that dependency subscription lists match the latest version.
          std::lock_guard lock(mutex_);
          if (version_ == version) {
            auto& new_deps = deps.deps();
            for (const auto& pair : deps_) {
              if (!new_deps.count(pair.first)) {
                pair.second->unsubscribe(key_);
              }
            }
            deps_.swap(new_deps);
            value_.swap(value);
          } else {
            for (const auto& pair : deps.deps()) {
              if (!deps_.count(pair.first)) {
                pair.second->unsubscribe(key_);
              }
            }
          }

          return *value_;
        };
  }

  ~ResourceGenerator() {
    std::lock_guard lock(mutex_);
    for (const auto& pair : deps_) {
      pair.second->unsubscribe(key_);
    }
  }

  // Returns the value atomically with caching.
  auto get() {
    if (auto ptr = std::atomic_load(&value_)) {
      return *ptr;
    } else {
      return generator_();
    }
  }

  // Returns key uniquely identifying this generator.
  uint64_t key() override {
    return key_;
  }

  // Atomically removes the resource with the given key as a subscriber.
  void unsubscribe(uint64_t resource_key) override {
    std::lock_guard lock(mutex_);
    subs_.erase(resource_key);
  }

  // Atomically adds the resource with the given key as a subscriber.
  void subscribe(uint64_t resource_key) override {
    std::lock_guard lock(mutex_);
    subs_.insert(resource_key);
  }

  // Atomically returns the resource keys of the current subscribers.
  std::unordered_set<uint64_t> subscribers() override {
    std::lock_guard lock(mutex_);
    return subs_;
  }

  // Atomically returns the resource keys of the current dependencies.
  std::unordered_set<uint64_t> dependencies() override {
    std::lock_guard lock(mutex_);
    std::unordered_set<uint64_t> deps;
    for (const auto& pair : deps_) {
      deps.insert(pair.first);
    }
    return deps;
  }

  // Update the resource version and returns the current subscribers.
  void build() override {
    generator_();
  }

  // Deletes the resource version and returns the current subscribers.
  void clear() override {
    std::lock_guard lock(mutex_);
    value_ = nullptr;
    deps_.clear();
    version_++;
  }

 private:
  uint64_t key_;
  std::function<Value()> generator_;
  std::atomic<uint64_t> version_;
  std::shared_ptr<Value> value_;
  std::unordered_map<uint64_t, std::shared_ptr<ResourceGeneratorBase>> deps_;
  std::unordered_set<uint64_t> subs_;
  std::mutex mutex_;
};

// TODO: Implement cache eviction.
// TODO: Implement hash collision fallback.
class Resources {
 public:
  Resources() : mutex_(std::make_unique<std::mutex>()) {}
  Resources(std::unordered_map<std::type_index, boost::any> overrides)
      : mutex_(std::make_unique<std::mutex>()),
        overrides_(std::move(overrides)) {}

  template <typename Resource, typename... Keys>
  auto generator(const Keys&... keys) {
    std::lock_guard guard(*mutex_);

    // Return the value from cache immediately if available.
    auto cache_key = resourceHash<Resource>(keys...);
    if (cache_.count(cache_key)) {
      using GeneratorType = ResourceGenerator<Resource, Keys...>;
      return std::static_pointer_cast<GeneratorType>(cache_.at(cache_key));
    }

    // Create generator and cache it.
    auto generator = makeGenerator<Resource>(keys...);
    ENFORCE(cache_.emplace(cache_key, generator).second);
    return generator;
  }

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) {
    return generator<Resource>(keys...)->get();
  }

  template <typename Resource, typename... Keys>
  void update(const Keys&... keys) {
    propagate(resourceHash<Resource>(keys...), [](auto generator) {
      generator->build();
    });
  }

  template <typename Resource, typename... Keys>
  void invalidate(const Keys&... keys) {
    propagate(resourceHash<Resource>(keys...), [](auto generator) {
      generator->clear();
    });
  }

 private:
  template <typename Function>
  void propagate(uint64_t source_key, Function&& fn) {
    auto gens = subscribers(source_key);

    // Compute the dependency subgraph of the generators.
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> subs;
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> deps;
    for (auto& pair : gens) {
      for (auto dep : pair.second->dependencies()) {
        if (gens.count(dep)) {
          deps[pair.second->key()].insert(dep);
        }
      }
      for (auto sub : pair.second->subscribers()) {
        if (gens.count(sub)) {
          subs[pair.second->key()].insert(sub);
        }
      }
    }

    // Propagate to the generators in topological order.
    std::vector<std::shared_ptr<ResourceGeneratorBase>> srcs;
    for (const auto& pair : gens) {
      if (!deps.count(pair.first)) {
        srcs.push_back(pair.second);
      }
    }
    while (!srcs.empty()) {
      auto src = srcs.back();
      srcs.pop_back();

      // Remove this generator as a dependency and update sources.
      for (auto sub : subs[src->key()]) {
        deps[sub].erase(src->key());
        if (deps[sub].empty() && gens.count(sub)) {
          srcs.push_back(gens.at(sub));
        }
      }

      // Propagate the given function on this generator.
      fn(src);
    }
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
        std::lock_guard guard(*mutex_);
        if (cache_.count(key)) {
          return cache_.at(key);
        } else {
          return std::shared_ptr<ResourceGeneratorBase>();
        }
      }();

      // Invalidate the generator and add its subs to the stack.
      if (generator) {
        ret[key] = generator;
        for (auto sub : generator->subscribers()) {
          if (!done_keys.count(sub)) {
            key_stack.push_back(sub);
          }
        }
      }
    }

    return ret;
  }

  template <typename Resource, typename... Keys>
  auto makeGenerator(const Keys&... keys) {
    using Fun = decltype(make_function(&Resource::operator()));
    using Gen = ResourceGenerator<Resource, Keys...>;
    auto it = overrides_.find(std::type_index(typeid(Resource)));
    if (it != overrides_.end()) {
      auto fun = boost::any_cast<Fun>(it->second);
      return std::make_shared<Gen>(*this, fun, keys...);
    } else {
      auto fun = [](auto&&... args) { return Resource()(args...); };
      return std::make_shared<Gen>(*this, fun, keys...);
    }
  }

  std::unique_ptr<std::mutex> mutex_;
  std::unordered_map<std::type_index, boost::any> overrides_;
  std::unordered_map<uint64_t, std::shared_ptr<ResourceGeneratorBase>> cache_;
};

class AsyncResources {
 public:
  AsyncResources(Resources resources, std::shared_ptr<QueueExecutor> executor)
      : resources_(std::move(resources)),
        executor_(std::move(executor)),
        semaphore_(0) {}

  ~AsyncResources() {
    // Spin until all asynchronous tasks are complete.
    while (semaphore_ > 0) {
    }
  }

  Resources& resources() {
    return resources_;
  }

  template <typename Resource, typename... Keys>
  auto get(const Keys&... keys) {
    return schedule(
        [this](const auto&... keys) {
          return resources().get<Resource>(keys...);
        },
        keys...);
  }

  template <typename Resource, typename... Keys>
  auto update(const Keys&... keys) {
    return schedule(
        [this](const auto&... keys) {
          return resources().update<Resource>(keys...);
        },
        keys...);
  }

  template <typename Resource, typename... Keys>
  auto invalidate(const Keys&... keys) {
    return schedule(
        [this](const auto&... keys) {
          return resources().invalidate<Resource>(keys...);
        },
        keys...);
  }

 private:
  template <typename Function, typename... Keys>
  auto schedule(Function fn, const Keys&... keys) {
    semaphore_ += 1;
    return executor_->schedule(
        [this, fn = std::move(fn), keys = std::make_tuple(keys...)] {
          Finally finally([&] { semaphore_ -= 1; });
          try {
            return std::apply(fn, keys);
          } catch (const std::exception& e) {
            LOG_ERROR(format("Error in async resources = %1%", e.what()));
            throw;
          }
        });
  }

  Resources resources_;
  std::shared_ptr<QueueExecutor> executor_;
  std::atomic<int> semaphore_;
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
  ResourcesBuilder& withSeed(ValueType&& value) {
    decltype(make_function(Resource())) fn =
        [value = std::forward<ValueType>(value)](
            const ResourceDeps& resources, auto... args) {
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
  Value operator()(const ResourceDeps& deps) {
    throwError("Missing seed resource: %1%", typeid(Resource).name());
  }
};

template <typename Resource, typename... Keys>
class ResourceMutation {
 public:
  ResourceMutation(Resources& resources, const Keys&... keys)
      : resources_(resources), value_(resources_.get<Resource>(keys...)) {
    invalidator_ = [this, keys...] {
      resources_.invalidate<Resource>(keys...);
    };
  }

  ~ResourceMutation() {
    invalidator_();
  }

  auto operator-> () {
    return value_;
  }

 private:
  Resources& resources_;
  decltype(resources_.get<Resource>(std::declval<Keys>()...)) value_;
  std::function<void()> invalidator_;
};

}  // namespace tequila