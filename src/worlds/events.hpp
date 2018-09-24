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

}  // namespace tequila