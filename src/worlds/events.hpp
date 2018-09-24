#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/js.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/spatial.hpp"
#include "src/common/voxels.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/scripts.hpp"

namespace tequila {

class EventHandler {
 public:
  EventHandler(
      std::shared_ptr<Window> window,
      std::shared_ptr<ScriptExecutor> scripts,
      std::shared_ptr<Resources> resources)
      : window_(window), scripts_(scripts), resources_(resources) {
    // Register key event callback.
    window_->on<glfwSetKeyCallback>(
        [&](int key, int scancode, int action, int mods) {
          if (key == GLFW_KEY_ESCAPE) {
            window_->close();
          } else if (key == GLFW_KEY_F) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_FILL);
          } else if (key == GLFW_KEY_G) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_LINE);
          }
          scripts_->delegate("on_key", key, scancode, action, mods);
        });

    // Register window resize event callback.
    window_->on<glfwSetFramebufferSizeCallback>([&](int width, int height) {
      int w, h;
      window_->call<glfwGetFramebufferSize>(&w, &h);
      if (width > 0 && height > 0) {
        gl::glViewport(0, 0, w, h);
        auto camera = ResourceMutation<WorldCamera>(*resources_);
        camera->aspect = static_cast<float>(width) / height;
      }
    });
  }

  ~EventHandler() {
    window_->clear<glfwSetFramebufferSizeCallback>();
    window_->clear<glfwSetKeyCallback>();
  }

  void onUpdate(float dt) {
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

/*
class WorldHandler {
 public:
  WorldHandler(std::shared_ptr<Window> window, Resources& resources)
      : window_(std::move(window)), resources_(resources) {}

  ~WorldHandler() {
    window_->clear<glfwSetKeyCallback>();
    window_->clear<glfwSetWindowSizeCallback>();
  }

  void update(float dt) {
    // Adjust the camera based on keyboard input.
    {
      auto key_press = [&](auto key) {
        return window_->call<glfwGetKey>(key) == GLFW_PRESS;
      };

      auto speed = 10.0f;
      auto up = glm::vec3(0.0f, 1.0f, 0.0f);
      if (key_press(GLFW_KEY_W)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position += speed * dt * camera->view;
      }
      if (key_press(GLFW_KEY_S)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position -= speed * dt * camera->view;
      }
      if (key_press(GLFW_KEY_D)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position += speed * dt * glm::cross(camera->view, up);
      }
      if (key_press(GLFW_KEY_A)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position -= speed * dt * glm::cross(camera->view, up);
      }
      if (key_press(GLFW_KEY_PAGE_UP)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position += speed * dt * up;
      }
      if (key_press(GLFW_KEY_PAGE_DOWN)) {
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->position -= speed * dt * up;
      }
    }

    // Adjust the camera based on mouse input.
    if (window_->call<glfwGetMouseButton>(GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
      static float theta = 0.0f;
      static float phi = 0.0f;

      int frame_width, frame_height;
      window_->call<glfwGetFramebufferSize>(&frame_width, &frame_height);

      double cursor_x, cursor_y;
      window_->call<glfwGetCursorPos>(&cursor_x, &cursor_y);

      // Compute the rotation angles in each direction.
      float speed = 0.1f;
      theta += dt * speed * static_cast<int>(0.5 * frame_width - cursor_x);
      phi += dt * speed * static_cast<int>(0.5 * frame_height - cursor_y);
      phi = std::clamp(phi, glm::radians(-45.0f), glm::radians(45.0f));

      auto camera = ResourceMutation<WorldCamera>(resources_);
      camera->view[0] = cos(phi) * sin(theta);
      camera->view[1] = sin(phi);
      camera->view[2] = cos(phi) * cos(theta);

      window_->call<glfwSetCursorPos>(0.5 * frame_width, 0.5 * frame_height);
      window_->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
      window_->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }

 private:
  std::shared_ptr<Window> window_;
  Resources& resources_;
};
*/

}  // namespace tequila