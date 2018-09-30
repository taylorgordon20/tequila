#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "src/common/errors.hpp"

namespace tequila {

class Registry;

template <typename InstanceType>
inline constexpr auto instanceKey() {
  return std::type_index(typeid(InstanceType));
}

template <typename InstanceType, typename StringType>
inline auto throwInstanceError(const StringType& msg) {
  throwError(
      "Registry error for type %1%: %2%", typeid(InstanceType).name(), msg);
}

template <typename InstanceType>
inline std::shared_ptr<InstanceType> gen(const Registry&) {
  throwInstanceError<InstanceType>("No default factory for type");
}

// Abstact base class for all providers. Exposes virtual methods for operations
// that need to be applied dynamically to each provider.
class ProviderBase {
 public:
  virtual ~ProviderBase() = default;
  virtual void prepare(const Registry&) = 0;
  virtual std::unique_ptr<ProviderBase> clone() const = 0;
};

// Providers wrap factory functions with memoization.
template <typename InstanceType>
class Provider : public ProviderBase {
  using InstancePtr = std::shared_ptr<InstanceType>;
  using ProviderFn = std::function<InstancePtr(const Registry&)>;

 public:
  Provider(ProviderFn provider_fn)
      : provider_fn_(std::move(provider_fn)), instance_(nullptr) {}

  void prepare(const Registry& registry) override {
    get(registry);
  }

  InstancePtr get(const Registry& registry) const {
    if (!instance_) {
      instance_ = provider_fn_(registry);
    }
    return instance_;
  }

  std::unique_ptr<ProviderBase> clone() const override {
    return std::make_unique<Provider<InstanceType>>(
        [instance = instance_](const Registry&) { return instance; });
  }

 private:
  ProviderFn provider_fn_;
  mutable InstancePtr instance_;
};

// Provides dependency injection via a factory pattern.
class Registry {
 public:
  template <typename InstanceType>
  std::shared_ptr<InstanceType> get() const {
    try {
      auto provider = providers_.at(instanceKey<InstanceType>()).get();
      return static_cast<Provider<InstanceType>*>(provider)->get(*this);
    } catch (const std::exception& e) {
      throwInstanceError<InstanceType>("Unbound registry key");
      return nullptr;
    }
  }

 private:
  std::unordered_map<std::type_index, std::unique_ptr<ProviderBase>> providers_;
  friend class RegistryBuilder;
};

// Provides a fluent-API to build a registry.
class RegistryBuilder {
 public:
  template <typename InstanceType>
  RegistryBuilder& bind(std::shared_ptr<InstanceType> instance) {
    return bind<InstanceType>(
        [instance = std::move(instance)](const Registry&) { return instance; });
  }

  template <typename InstanceType, typename FunctionType>
  RegistryBuilder& bind(FunctionType fn) {
    auto provider = std::make_unique<Provider<InstanceType>>(std::move(fn));
    registry_.providers_[instanceKey<InstanceType>()] = std::move(provider);
    return *this;
  }

  template <typename InstanceType>
  RegistryBuilder& bindToDefaultFactory() {
    return bind<InstanceType>(gen<InstanceType>);
  }

  template <typename InstanceType>
  RegistryBuilder& bindAll(const Registry& other) {
    for (const auto& pair : other.providers_) {
      registry_.providers_[pair.first] = pair.second->clone();
    }
    return *this;
  }

  Registry build() {
    // Eagerly prepare each dependency.
    Registry ret;
    ret.providers_.swap(registry_.providers_);
    for (const auto& pair : ret.providers_) {
      pair.second->prepare(ret);
    }
    return ret;
  };

 private:
  Registry registry_;
};

}  // namespace tequila
