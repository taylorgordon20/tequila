#pragma once

#include <memory>
#include <string>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/files.hpp"
#include "src/common/functions.hpp"
#include "src/common/js.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

struct WorldInputScript {
  auto operator()(const Resources& resources) {
    constexpr auto kScript = "scripts/world_input.js";
    return std::make_shared<JsModule>(
        resources.get<WorldJsContext>(), kScript, loadFile(kScript));
  }
};

auto FFI_get_camera_pos(std::shared_ptr<Resources>& resources) {
  return make_function([resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<double>{
        camera->position[0], camera->position[1], camera->position[2]};
  });
}

auto FFI_set_camera_pos(std::shared_ptr<Resources>& resources) {
  return make_function([resources](double x, double y, double z) {
    auto camera = ResourceMutation<WorldCamera>(*resources);
    camera->position[0] = x;
    camera->position[1] = y;
    camera->position[2] = z;
  });
}

auto FFI_get_camera_view(std::shared_ptr<Resources>& resources) {
  return make_function([resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<double>{
        camera->view[0], camera->view[1], camera->view[2]};
  });
}

auto FFI_set_camera_view(std::shared_ptr<Resources>& resources) {
  return make_function([resources](double x, double y, double z) {
    auto camera = ResourceMutation<WorldCamera>(*resources);
    camera->view[0] = x;
    camera->view[1] = y;
    camera->view[2] = z;
  });
}

auto FFI_is_key_pressed(std::shared_ptr<Window>& window) {
  return make_function([window](int key) {
    return window->call<glfwGetKey>(key) == GLFW_PRESS;
  });
}

class ScriptExecutor {
 public:
  ScriptExecutor(
      std::shared_ptr<Window> window, std::shared_ptr<Resources> resources)
      : resources_(resources) {
    auto ctx = resources_->get<WorldJsContext>();
    ctx->setGlobal("get_camera_pos", FFI_get_camera_pos(resources));
    ctx->setGlobal("set_camera_pos", FFI_set_camera_pos(resources));
    ctx->setGlobal("get_camera_view", FFI_get_camera_view(resources));
    ctx->setGlobal("set_camera_view", FFI_set_camera_view(resources));
    ctx->setGlobal("is_key_pressed", FFI_is_key_pressed(window));
  }

  template <typename... Args>
  void delegate(const std::string& event, Args&&... args) {
    // Delegate the event call to all active scripts.
    std::vector<std::shared_ptr<JsModule>> js_modules;
    js_modules.push_back(resources_->get<WorldInputScript>());
    for (const auto& module : js_modules) {
      if (module->has(event)) {
        module->call<void>(event, args...);
      }
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<ScriptExecutor> gen(const Registry& registry) {
  return std::make_shared<ScriptExecutor>(
      registry.get<Window>(), registry.get<Resources>());
}

}  // namespace tequila
