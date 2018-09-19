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
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/spatial.hpp"
#include "src/common/voxels.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

class WorldHandler {
 public:
  WorldHandler(std::shared_ptr<Window> window, Resources& resources)
      : window_(std::move(window)), resources_(resources) {
    // Register some debugging-related keyboard events.
    window_->on<glfwSetKeyCallback>(
        [this](int key, int scancode, int action, int mods) {
          if (key == GLFW_KEY_ESCAPE) {
            window_->close();
          } else if (key == GLFW_KEY_F) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_FILL);
          } else if (key == GLFW_KEY_G) {
            gl::glPolygonMode(gl::GL_FRONT_AND_BACK, gl::GL_LINE);
          }
        });

    // Register the window-resize event.
    window_->on<glfwSetWindowSizeCallback>([this](int width, int height) {
      int w, h;
      window_->call<glfwGetFramebufferSize>(&w, &h);
      if (width > 0 && height > 0) {
        gl::glViewport(0, 0, w, h);
        auto camera = ResourceMutation<WorldCamera>(resources_);
        camera->aspect = static_cast<float>(width) / height;
      }
    });
  }

  ~WorldHandler() {
    window_->clear<glfwSetKeyCallback>();
    window_->clear<glfwSetWindowSizeCallback>();
  }

  void update(float dt) const {
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

}  // namespace tequila