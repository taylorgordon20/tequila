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
      std::shared_ptr<Stats> stats,
      std::shared_ptr<Window> window,
      std::shared_ptr<ScriptExecutor> scripts,
      std::shared_ptr<Resources> resources)
      : stats_(stats),
        window_(window),
        scripts_(scripts),
        resources_(resources) {
    // Register window resize event callback.
    window_->on<glfwSetFramebufferSizeCallback>([&](int width, int height) {
      StatsTimer timer(stats_, "events.on_resize");
      if (width > 0 && height > 0) {
        auto camera = ResourceMutation<WorldCamera>(*resources_);
        camera->aspect = static_cast<float>(width) / height;
      }
      scripts_->delegate("on_resize", width, height);
    });

    // Register key event callback.
    window_->on<glfwSetKeyCallback>([&](int key,
                                        int scancode,
                                        int action,
                                        int mods) {
      StatsTimer timer(stats_, "events.on_key");
      if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F1) {
          gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_FILL);
        } else if (key == GLFW_KEY_F2) {
          gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_LINE);
        } else if (key == GLFW_KEY_ENTER && mods == GLFW_MOD_ALT) {
          if (window_->call<glfwGetWindowMonitor>()) {
            // Leave fullscreen.
            window_->call<glfwSetWindowMonitor>(
                (GLFWmonitor*)nullptr, 100, 100, 1024, 768, 0);
          } else {
            // Enter fullscreen.
            auto monitor = glfwGetPrimaryMonitor();
            if (monitor) {
              const auto mode = glfwGetVideoMode(monitor);
              window_->call<glfwSetWindowMonitor>(
                  monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
              glfwSwapInterval(1);
            }
          }
        }
      }
      scripts_->delegate("on_key", key, scancode, action, mods);
    });

    // Register scroll event callback.
    window_->on<glfwSetCharCallback>([&](unsigned int codepoint) {
      StatsTimer timer(stats_, "events.on_text");
      scripts_->delegate("on_text", codepoint);
    });

    // Register scroll event callback.
    window_->on<glfwSetScrollCallback>([&](double x_offset, double y_offset) {
      StatsTimer timer(stats_, "events.on_scroll");
      scripts_->delegate("on_scroll", x_offset, y_offset);
    });

    // Register click event callback.
    window_->on<glfwSetMouseButtonCallback>(
        [&](int button, int action, int mods) {
          StatsTimer timer(stats_, "events.on_click");
          scripts_->delegate("on_click", button, action, mods);
        });
  }

  ~EventHandler() {
    window_->clear<glfwSetFramebufferSizeCallback>();
    window_->clear<glfwSetKeyCallback>();
  }

  void update(float dt) {
    StatsTimer timer(stats_, "events.on_update");
    scripts_->delegate("on_update", dt);
  }

 private:
  std::shared_ptr<Stats> stats_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<ScriptExecutor> scripts_;
  std::shared_ptr<Resources> resources_;
};

template <>
inline std::shared_ptr<EventHandler> gen(const Registry& registry) {
  return std::make_shared<EventHandler>(
      registry.get<Stats>(),
      registry.get<Window>(),
      registry.get<ScriptExecutor>(),
      registry.get<Resources>());
}

}  // namespace tequila