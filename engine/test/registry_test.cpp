#include <iostream>
#include <memory>
#include <string>

#include "engine/common/registry.hpp"

namespace tequila {

struct Foo {
  int x;
  int y;
};

struct Bar {
  std::shared_ptr<Foo> foo;
  Bar(std::shared_ptr<Foo> foo) : foo(std::move(foo)) {}
};

struct Jimmy {
  std::shared_ptr<Foo> foo;
  std::shared_ptr<Bar> bar;
  Jimmy(std::shared_ptr<Foo> foo, std::shared_ptr<Bar> bar)
      : foo(std::move(foo)), bar(std::move(bar)) {}
};

auto jimmyFactory(const Registry& registry) {
  return std::make_shared<Jimmy>(registry.get<Foo>(), registry.get<Bar>());
}

void run() {
  auto foo = std::make_shared<Foo>();
  foo->x = 1;
  foo->y = 2;

  auto bar_provider = [](const Registry& registry) {
    return std::make_shared<Bar>(registry.get<Foo>());
  };

  auto registry = RegistryBuilder()
                      .bind<Foo>(std::move(foo))
                      .bind<Bar>(std::move(bar_provider))
                      .bind<Jimmy>(jimmyFactory)
                      .build();

  std::cout << "Foo.x=" << registry.get<Foo>()->x << std::endl;
  std::cout << "Foo.y=" << registry.get<Foo>()->y << std::endl;
  std::cout << "Bar.foo.x=" << registry.get<Bar>()->foo->x << std::endl;
  std::cout << "Bar.foo.y=" << registry.get<Bar>()->foo->y << std::endl;
  std::cout << "Jimmy.foo.x=" << registry.get<Jimmy>()->foo->x << std::endl;
  std::cout << "Jimmy.foo.y=" << registry.get<Jimmy>()->foo->y << std::endl;
  std::cout << "Jimmy.bar.foo.x=" << registry.get<Jimmy>()->bar->foo->x
            << std::endl;
  std::cout << "Jimmy.bar.foo.y=" << registry.get<Jimmy>()->bar->foo->y
            << std::endl;
}

}  // namespace tequila

int main() {
  try {
    tequila::run();
  } catch (const std::exception& e) {
    std::cout << "Exception during execution: " << e.what() << std::endl;
  }
  return 0;
}