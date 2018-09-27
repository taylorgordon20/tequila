#pragma once

#include <memory>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/scripts.hpp"
#include "src/worlds/ui.hpp"

namespace tequila {

class EventHandler {
 public:
  EventHandler(
      std::shared_ptr<Window> window,
      std::shared_ptr<ScriptExecutor> scripts,
      std::shared_ptr<Resources> resources)
      : window_(window), scripts_(scripts), resources_(resources) {
    // Register window resize event callback.
    window_->on<glfwSetFramebufferSizeCallback>([&](int width, int height) {
      int w, h;
      window_->call<glfwGetFramebufferSize>(&w, &h);
      if (width > 0 && height > 0) {
        gl::glViewport(0, 0, w, h);
        auto camera = ResourceMutation<WorldCamera>(*resources_);
        camera->aspect = static_cast<float>(width) / height;
      }

      // Update the crosshair
      {
        auto ui = ResourceMutation<WorldUI>(*resources_);
        auto& node = ui->nodes["crosshair"];
        int crosshair_w = to<int>(node.attr["width"]);
        int crosshair_h = to<int>(node.attr["height"]);
        node.attr["x"] = to<std::string>(w / 2 - crosshair_w / 2);
        node.attr["y"] = to<std::string>(h / 2 - crosshair_h / 2);
      }
    });

    // Register key event callback.
    window_->on<glfwSetKeyCallback>(
        [&](int key, int scancode, int action, int mods) {
          if (key == GLFW_KEY_ESCAPE) {
            window_->close();
          } else if (key == GLFW_KEY_EQUAL) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_FILL);
          } else if (key == GLFW_KEY_MINUS) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_LINE);
          }
          scripts_->delegate("on_key", key, scancode, action, mods);
        });

    // Register click event callback.
    window_->on<glfwSetMouseButtonCallback>(
        [&](int button, int action, int mods) {
          scripts_->delegate("on_click", button, action, mods);
        });

    // Notify scripts the initialization event.
    scripts_->delegate("on_init");
  }

  ~EventHandler() {
    window_->clear<glfwSetFramebufferSizeCallback>();
    window_->clear<glfwSetKeyCallback>();
  }

  void update(float dt) {
    scripts_->delegate("on_update", dt);
  }

 private:
  std::shared_ptr<Window> window_;
  std::shared_ptr<ScriptExecutor> scripts_;
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<EventHandler> gen(const Registry& registry) {
  return std::make_shared<EventHandler>(
      registry.get<Window>(),
      registry.get<ScriptExecutor>(),
      registry.get<Resources>());
}

}  // namespace tequila