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
#include "src/common/utils.hpp"
#include "src/common/window.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/terrain.hpp"
#include "src/worlds/ui.hpp"

namespace tequila {

struct ScriptContext
    : SeedResource<ScriptContext, std::shared_ptr<LuaContext>> {};

struct ScriptModule {
  auto operator()(ResourceDeps& deps, const std::string& name) {
    try {
      auto code = loadFile(format("scripts/%1%.lua", name).c_str());
      return std::make_shared<LuaModule>(*deps.get<ScriptContext>(), code);
    } catch (const LuaError& error) {
      std::cout << "Error parsing script: " << name << ".lua" << std::endl;
      throw;
    }
  }
};

auto FFI_exit(std::shared_ptr<Window>& window) {
  return [window] { window->close(); };
}

auto FFI_now_time() {
  return [] {
    auto now = std::chrono::system_clock::now();
    auto dur = std::chrono::duration<double>(now.time_since_epoch());
    return dur.count();
  };
}

auto FFI_reload(std::shared_ptr<Resources>& resources) {
  return [resources] { resources->invalidate<ScriptContext>(); };
}

auto FFI_get_module(std::shared_ptr<Resources>& resources) {
  return [resources](const std::string& script) {
    return resources->get<ScriptModule>(script)->table();
  };
}

auto FFI_clear_stats(std::shared_ptr<Stats>& stats) {
  return [stats] { return stats->clear(); };
}

auto FFI_get_stats(std::shared_ptr<Stats>& stats) {
  return [stats] { return stats->keys(); };
}

auto FFI_get_stat_average(std::shared_ptr<Stats>& stats) {
  return [stats](const std::string& stat) {
    sol::optional<float> ret;
    if (stats->has(stat)) {
      ret = stats->getAverage(stat);
    }
    return ret;
  };
}

auto FFI_get_stat_maximum(std::shared_ptr<Stats>& stats) {
  return [stats](const std::string& stat) {
    sol::optional<float> ret;
    if (stats->has(stat)) {
      ret = stats->getMaximum(stat);
    }
    return ret;
  };
}

auto FFI_get_light_dir(std::shared_ptr<Resources>& resources) {
  return [resources] {
    auto light = resources->get<WorldLight>();
    return std::vector<float>{(*light)[0], (*light)[1], (*light)[2]};
  };
}

auto FFI_set_light_dir(std::shared_ptr<Resources>& resources) {
  return [resources](float x, float y, float z) {
    ResourceMutation<WorldLight> light(*resources);
    light->operator[](0) = x;
    light->operator[](1) = y;
    light->operator[](2) = z;
  };
}

auto FFI_get_camera_pos(std::shared_ptr<Resources>& resources) {
  return [resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<float>{
        camera->position[0], camera->position[1], camera->position[2]};
  };
}

auto FFI_set_camera_pos(std::shared_ptr<Resources>& resources) {
  return [resources](float x, float y, float z) {
    ResourceMutation<WorldCamera> camera(*resources);
    camera->position[0] = x;
    camera->position[1] = y;
    camera->position[2] = z;
  };
}

auto FFI_get_camera_view(std::shared_ptr<Resources>& resources) {
  return [resources] {
    auto camera = resources->get<WorldCamera>();
    return std::vector<float>{
        camera->view[0], camera->view[1], camera->view[2]};
  };
}

auto FFI_set_camera_view(std::shared_ptr<Resources>& resources) {
  return [resources](float x, float y, float z) {
    auto camera = ResourceMutation<WorldCamera>(*resources);
    camera->view[0] = x;
    camera->view[1] = y;
    camera->view[2] = z;
  };
}

auto FFI_set_style_config(std::shared_ptr<Resources>& resources) {
  return [resources](const std::string& name) {
    ResourceMutation<WorldStyleName>(*resources)->assign(name);
  };
}

auto FFI_is_key_pressed(std::shared_ptr<Window>& window) {
  return
      [window](int key) { return window->call<glfwGetKey>(key) == GLFW_PRESS; };
}

auto FFI_is_mouse_pressed(std::shared_ptr<Window>& window) {
  return [window](int button) {
    return window->call<glfwGetMouseButton>(button) == GLFW_PRESS;
  };
}

auto FFI_get_cursor_pos(std::shared_ptr<Window>& window) {
  return [window]() {
    std::vector<double> cursor_pos(2);
    window->call<glfwGetCursorPos>(&cursor_pos[0], &cursor_pos[1]);
    return cursor_pos;
  };
}

auto FFI_set_cursor_pos(std::shared_ptr<Window>& window) {
  return [window](float x, float y) { window->call<glfwSetCursorPos>(x, y); };
}

auto FFI_show_cursor(std::shared_ptr<Window>& window) {
  return [window](bool visible) {
    if (visible) {
      window->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
      window->call<glfwSetInputMode>(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
  };
}

auto FFI_get_window_size(std::shared_ptr<Window>& window) {
  return [window]() {
    std::vector<int> window_size(2);
    window->call<glfwGetFramebufferSize>(&window_size[0], &window_size[1]);
    return window_size;
  };
}

auto FFI_get_voxel(std::shared_ptr<VoxelsUtil>& voxels_util) {
  return [voxels_util](float x, float y, float z) {
    return voxels_util->getVoxel(x, y, z);
  };
}

auto FFI_set_voxel(std::shared_ptr<VoxelsUtil>& voxels_util) {
  return [voxels_util](float x, float y, float z, uint32_t value) {
    voxels_util->setVoxel(x, y, z, value);
  };
}

auto FFI_get_ray_voxels(std::shared_ptr<VoxelsUtil>& voxels_util) {
  return [voxels_util](
             float start_x,
             float start_y,
             float start_z,
             float dir_x,
             float dir_y,
             float dir_z,
             float distance) {
    std::vector<std::vector<int>> results;
    voxels_util->marchVoxels(
        glm::vec3(start_x, start_y, start_z),
        glm::vec3(dir_x, dir_y, dir_z),
        distance,
        [&](int ix, int iy, int iz, float distance) {
          results.emplace_back(std::vector<int>{ix, iy, iz});
          return true;
        });
    return results;
  };
}

auto FFI_create_ui_node(std::shared_ptr<Resources>& resources) {
  return [resources](std::string id, std::string kind, const sol::table& attr) {
    ResourceMutation<WorldUI> ui(*resources);
    ENFORCE(!ui->nodes.count(id), concat("ID already exists: ", id));
    auto& node = ui->nodes[id];
    node.kind = std::move(kind);
    auto lua = resources->get<ScriptContext>();
    attr.for_each([&](sol::object key, sol::object val) {
      node.attr[key.as<std::string>()] = lua->state()["tostring"](val);
    });
  };
}

auto FFI_update_ui_node(std::shared_ptr<Resources>& resources) {
  return [resources](std::string id, const sol::table& table) {
    ResourceMutation<WorldUI> ui(*resources);
    auto& node = ui->nodes.at(id);
    auto lua = resources->get<ScriptContext>();
    table.for_each([&](sol::object key, sol::object val) {
      node.attr[key.as<std::string>()] = lua->state()["tostring"](val);
    });
  };
}

auto FFI_delete_ui_node(std::shared_ptr<Resources>& resources) {
  return [resources](const std::string& id) {
    ResourceMutation<WorldUI> ui(*resources);
    ENFORCE(ui->nodes.erase(id));
  };
}

class ScriptExecutor {
 public:
  ScriptExecutor(
      std::shared_ptr<Window> window,
      std::shared_ptr<Resources> resources,
      std::shared_ptr<VoxelsUtil> voxels_util,
      std::shared_ptr<Stats> stats)
      : window_(window),
        resources_(resources),
        voxels_util_(voxels_util),
        stats_(stats) {}

  template <typename... Args>
  void delegate(const std::string& event, Args&&... args) {
    auto ctx = resources_->get<ScriptContext>();
    if (ctx->state()["__initialized"] == sol::lua_nil) {
      initializeFFI(*ctx);
    }

    // Delegate the event call to all active scripts.
    std::vector<std::shared_ptr<LuaModule>> lua_modules;
    lua_modules.push_back(resources_->get<ScriptModule>("debug"));
    lua_modules.push_back(resources_->get<ScriptModule>("console"));
    lua_modules.push_back(resources_->get<ScriptModule>("camera"));
    lua_modules.push_back(resources_->get<ScriptModule>("editor"));
    lua_modules.push_back(resources_->get<ScriptModule>("game"));
    for (const auto& module : lua_modules) {
      if (!module->has("__initialized")) {
        module->call<void>("on_init");
        module->deleter() = [](LuaModule* module) {
          if (module->has("on_done")) {
            try {
              module->call<void>("on_done");
            } catch (const LuaError& e) {
              std::cout << "Bad 'on_done' function: " << e.what() << std::endl;
            }
          }
        };
        module->set("__initialized", true);
      }
      if (module->has(event)) {
        auto ret = module->call<sol::optional<bool>>(event, args...);
        if (ret && *ret) {
          break;
        }
      }
    }
  }

 private:
  // TODO: Instead of a lambda consider passing a custom type with doc.
  template <typename Return, typename... Args>
  auto wrapImpl(std::function<Return(Args...)> fn) {
    return [fn = std::move(fn)](sol::this_state s, const Args&... args) {
      try {
        return fn(args...);
      } catch (const std::exception& e) {
        lua_State* L = s;
        luaL_error(L, "Error: %s", e.what());
        return makeDefault<Return>();
      }
    };
  }

  template <typename Function>
  auto wrapFFI(Function&& fn) {
    return wrapImpl(make_function(std::forward<Function>(fn)));
  }

  void initializeFFI(LuaContext& ctx) {
    std::cout << "Intializing Lua FFI." << std::endl;
    ctx.set("exit", wrapFFI(FFI_exit(window_)));
    ctx.set("now_time", wrapFFI(FFI_now_time()));
    ctx.set("reload", wrapFFI(FFI_reload(resources_)));
    ctx.set("get_module", wrapFFI(FFI_get_module(resources_)));
    ctx.set("clear_stats", wrapFFI(FFI_clear_stats(stats_)));
    ctx.set("get_stats", wrapFFI(FFI_get_stats(stats_)));
    ctx.set("get_stat_average", wrapFFI(FFI_get_stat_average(stats_)));
    ctx.set("get_stat_maximum", wrapFFI(FFI_get_stat_maximum(stats_)));
    ctx.set("get_light_dir", wrapFFI(FFI_get_light_dir(resources_)));
    ctx.set("set_light_dir", wrapFFI(FFI_set_light_dir(resources_)));
    ctx.set("get_camera_pos", wrapFFI(FFI_get_camera_pos(resources_)));
    ctx.set("set_camera_pos", wrapFFI(FFI_set_camera_pos(resources_)));
    ctx.set("get_camera_view", wrapFFI(FFI_get_camera_view(resources_)));
    ctx.set("set_camera_view", wrapFFI(FFI_set_camera_view(resources_)));
    ctx.set("set_style_config", wrapFFI(FFI_set_style_config(resources_)));
    ctx.set("is_key_pressed", wrapFFI(FFI_is_key_pressed(window_)));
    ctx.set("is_mouse_pressed", wrapFFI(FFI_is_mouse_pressed(window_)));
    ctx.set("get_cursor_pos", wrapFFI(FFI_get_cursor_pos(window_)));
    ctx.set("set_cursor_pos", wrapFFI(FFI_set_cursor_pos(window_)));
    ctx.set("show_cursor", wrapFFI(FFI_show_cursor(window_)));
    ctx.set("get_window_size", wrapFFI(FFI_get_window_size(window_)));
    ctx.set("get_voxel", wrapFFI(FFI_get_voxel(voxels_util_)));
    ctx.set("set_voxel", wrapFFI(FFI_set_voxel(voxels_util_)));
    ctx.set("get_ray_voxels", wrapFFI(FFI_get_ray_voxels(voxels_util_)));
    ctx.set("create_ui_node", wrapFFI(FFI_create_ui_node(resources_)));
    ctx.set("update_ui_node", wrapFFI(FFI_update_ui_node(resources_)));
    ctx.set("delete_ui_node", wrapFFI(FFI_delete_ui_node(resources_)));
    ctx.state()["__initialized"] = true;
  }

  std::shared_ptr<Window> window_;
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<VoxelsUtil> voxels_util_;
  std::shared_ptr<Stats> stats_;
};

template <>
inline std::shared_ptr<ScriptExecutor> gen(const Registry& registry) {
  return std::make_shared<ScriptExecutor>(
      registry.get<Window>(),
      registry.get<Resources>(),
      registry.get<VoxelsUtil>(),
      registry.get<Stats>());
}

}  // namespace tequila
