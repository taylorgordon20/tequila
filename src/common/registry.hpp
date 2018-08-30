#pragma once

#include <boost/format.hpp>
#include <functional>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace tequila {

class Registry;

template <typename InstanceType>
constexpr auto instanceKey() {
  return std::type_index(typeid(InstanceType));
}

// Abstact base class for all providers. Exposes virtual methods for operations
// that need to be applied dynamically to each provider.
class ProviderBase {
 public:
  virtual void prepare(const Registry&) = 0;
};

// Providers wrap factory functions with memoization.
template <typename InstanceType>
class Provider : public ProviderBase {
  using InstancePtr = std::shared_ptr<InstanceType>;
  using ProviderFn = std::function<InstancePtr(const Registry&)>;

 public:
  Provider(ProviderFn provider_fn)
      : provider_fn_(std::move(provider_fn)), instance_(nullptr) {}

  InstancePtr get(const Registry& registry) const {
    if (!instance_) {
      instance_ = provider_fn_(registry);
    }
    return instance_;
  }

  void prepare(const Registry& registry) override { get(registry); }

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
      boost::format error("Unbound registry key: %1%");
      error % typeid(InstanceType).name();
      throw std::runtime_error(error.str().c_str());
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
