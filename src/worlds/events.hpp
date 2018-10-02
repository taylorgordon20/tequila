#pragma once

#include <memory>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/window.hpp"
#include "src/worlds/console.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/scripts.hpp"
#include "src/worlds/ui.hpp"

namespace tequila {

class EventHandler {
 public:
  EventHandler(
      std::shared_ptr<Console> console,
      std::shared_ptr<Window> window,
      std::shared_ptr<ScriptExecutor> scripts,
      std::shared_ptr<Resources> resources)
      : console_(console),
        window_(window),
        scripts_(scripts),
        resources_(resources) {
    // Register window resize event callback.
    window_->on<glfwSetFramebufferSizeCallback>([&](int width, int height) {
      int w, h;
      window_->call<glfwGetFramebufferSize>(&w, &h);
      if (width > 0 && height > 0) {
        gl::glViewport(0, 0, w, h);
        auto camera = ResourceMutation<WorldCamera>(*resources_);
        camera->aspect = static_cast<float>(width) / height;
      }
      scripts_->delegate("on_resize", width, height);
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

    // Register scroll event callback.
    window_->on<glfwSetScrollCallback>([&](double x_offset, double y_offset) {
      scripts_->delegate("on_scroll", x_offset, y_offset);
    });

    // Register click event callback.
    window_->on<glfwSetMouseButtonCallback>(
        [&](int button, int action, int mods) {
          scripts_->delegate("on_click", button, action, mods);
        });

    // Registry console line callback.
    console_->onLine([](std::string line) {
      std::cout << "Received input: \"" << line << "\"" << std::endl;
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
  std::shared_ptr<Console> console_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<ScriptExecutor> scripts_;
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<EventHandler> gen(const Registry& registry) {
  return std::make_shared<EventHandler>(
      registry.get<Console>(),
      registry.get<Window>(),
      registry.get<ScriptExecutor>(),
      registry.get<Resources>());
}

}  // namespace tequila