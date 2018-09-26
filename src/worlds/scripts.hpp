#pragma once

#include <memory>
#include <string>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/files.hpp"
#include "src/common/functions.hpp"
#include "src/common/lua.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/terrain.hpp"

namespace tequila {

struct ScriptContext
    : SeedResource<ScriptContext, std::shared_ptr<LuaContext>> {};

struct WorldInputScript {
  auto operator()(const Resources& resources) {
    return std::make_shared<LuaModule>(
        *resources.get<ScriptContext>(), loadFile("scripts/world_input.lua"));
  }
};

auto FFI_get_camera_pos(std::shared_ptr<Resources>& resources) {
  return sol::as_function([resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<float>{
        camera->position[0], camera->position[1], camera->position[2]};
  });
}

auto FFI_set_camera_pos(std::shared_ptr<Resources>& resources) {
  return sol::as_function([resources](float x, float y, float z) {
    auto camera = ResourceMutation<WorldCamera>(*resources);
    camera->position[0] = x;
    camera->position[1] = y;
    camera->position[2] = z;
  });
}

auto FFI_get_camera_view(std::shared_ptr<Resources>& resources) {
  return sol::as_function([resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<float>{
        camera->view[0], camera->view[1], camera->view[2]};
  });
}

auto FFI_set_camera_view(std::shared_ptr<Resources>& resources) {
  return sol::as_function([resources](float x, float y, float z) {
    auto camera = ResourceMutation<WorldCamera>(*resources);
    camera->view[0] = x;
    camera->view[1] = y;
    camera->view[2] = z;
  });
}

auto FFI_is_key_pressed(std::shared_ptr<Window>& window) {
  return sol::as_function([window](int key) {
    return window->call<glfwGetKey>(key) == GLFW_PRESS;
  });
}

auto FFI_is_mouse_pressed(std::shared_ptr<Window>& window) {
  return sol::as_function([window](int button) {
    return window->call<glfwGetMouseButton>(button) == GLFW_PRESS;
  });
}

auto FFI_get_cursor_pos(std::shared_ptr<Window>& window) {
  return sol::as_function([window]() {
    std::vector<double> cursor_pos(2);
    window->call<glfwGetCursorPos>(&cursor_pos[0], &cursor_pos[1]);
    return cursor_pos;
  });
}

auto FFI_set_cursor_pos(std::shared_ptr<Window>& window) {
  return sol::as_function(
      [window](float x, float y) { window->call<glfwSetCursorPos>(x, y); });
}

auto FFI_set_cursor_visible(std::shared_ptr<Window>& window) {
  return sol::as_function([window](bool visible) {
    if (visible) {
      window->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
      window->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
  });
}

auto FFI_get_window_size(std::shared_ptr<Window>& window) {
  return sol::as_function([window]() {
    std::vector<int> window_size(2);
    window->call<glfwGetFramebufferSize>(&window_size[0], &window_size[1]);
    return window_size;
  });
}

auto FFI_get_voxel(std::shared_ptr<TerrainUtil>& terrain_util) {
  return sol::as_function([terrain_util](float x, float y, float z) {
    return terrain_util->getVoxel(x, y, z);
  });
}

auto FFI_set_voxel(std::shared_ptr<TerrainUtil>& terrain_util) {
  return sol::as_function(
      [terrain_util](float x, float y, float z, uint32_t value) {
        terrain_util->setVoxel(x, y, z, value);
      });
}

auto FFI_get_ray_voxels(std::shared_ptr<TerrainUtil>& terrain_util) {
  return sol::as_function([terrain_util](
                              float start_x,
                              float start_y,
                              float start_z,
                              float dir_x,
                              float dir_y,
                              float dir_z,
                              float distance) {
    std::vector<std::vector<int>> results;
    terrain_util->marchVoxels(
        glm::vec3(start_x, start_y, start_z),
        glm::vec3(dir_x, dir_y, dir_z),
        distance,
        [&](int ix, int iy, int iz) {
          results.emplace_back(std::vector<int>{ix, iy, iz});
        });
    return results;
  });
}

class ScriptExecutor {
 public:
  ScriptExecutor(
      std::shared_ptr<Window> window,
      std::shared_ptr<Resources> resources,
      std::shared_ptr<TerrainUtil> terrain_util)
      : resources_(resources) {
    auto ctx = resources_->get<ScriptContext>();
    ctx->set("get_camera_pos", FFI_get_camera_pos(resources));
    ctx->set("set_camera_pos", FFI_set_camera_pos(resources));
    ctx->set("get_camera_view", FFI_get_camera_view(resources));
    ctx->set("set_camera_view", FFI_set_camera_view(resources));
    ctx->set("is_key_pressed", FFI_is_key_pressed(window));
    ctx->set("is_mouse_pressed", FFI_is_mouse_pressed(window));
    ctx->set("get_cursor_pos", FFI_get_cursor_pos(window));
    ctx->set("set_cursor_pos", FFI_set_cursor_pos(window));
    ctx->set("set_cursor_visible", FFI_set_cursor_visible(window));
    ctx->set("get_window_size", FFI_get_window_size(window));
    ctx->set("get_voxel", FFI_get_voxel(terrain_util));
    ctx->set("set_voxel", FFI_set_voxel(terrain_util));
    ctx->set("get_ray_voxels", FFI_get_ray_voxels(terrain_util));
  }

  template <typename... Args>
  void delegate(const std::string& event, Args&&... args) {
    // Delegate the event call to all active scripts.
    std::vector<std::shared_ptr<LuaModule>> lua_modules;
    lua_modules.push_back(resources_->get<WorldInputScript>());
    for (const auto& module : lua_modules) {
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
      registry.get<Window>(),
      registry.get<Resources>(),
      registry.get<TerrainUtil>());
}

}  // namespace tequila
