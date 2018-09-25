#include <iostream>
#include <string>
#include <utility>

#ifdef _WIN32
#include "windows.h"
#endif

#include "src/common/errors.hpp"
#include "src/common/lua.hpp"
#include "src/common/opengl.hpp"
#include "src/common/resources.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/events.hpp"
#include "src/worlds/scripts.hpp"
#include "src/worlds/terrain.hpp"

namespace tequila {

auto makeWorldCamera() {
  auto camera = std::make_shared<Camera>();
  camera->position = glm::vec3(0.0f, 0.0f, 0.0f);
  camera->view = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
  camera->fov = glm::radians(45.0f);
  camera->aspect = 4.0f / 3.0f;
  camera->near_distance = 0.1f;
  camera->far_distance = 100.0f;
  return camera;
}

auto makeWorldLight() {
  return std::make_shared<glm::vec3>(
      glm::normalize(glm::vec3(-2.0f, 4.0f, 1.0f)));
}

auto makeScriptContext() {
  return std::make_shared<LuaContext>();
}

auto makeWorldResources(const std::string& world_name) {
  return std::make_shared<Resources>(
      ResourcesBuilder()
          .withSeed<ScriptContext>(makeScriptContext())
          .withSeed<WorldCamera>(makeWorldCamera())
          .withSeed<WorldLight>(makeWorldLight())
          .withSeed<WorldName>(world_name)
          .build());
}

void run() {
  // Figure out what world to load.
  std::string world_name;
  std::cout << "Enter world name (e.g. octree_world): ";
  std::getline(std::cin, world_name);
  if (world_name.empty()) {
    std::cout << "Defaulting to world 'octree_world'." << std::endl;
    world_name = "octree_world";
  }

  // Initialize game registry.
  Application app;
  Registry registry =
      RegistryBuilder()
          .bind<Window>(app.makeWindow(1024, 768, "Tequila!", nullptr, nullptr))
          .bind<Resources>(makeWorldResources(world_name))
          .bindToDefaultFactory<EventHandler>()
          .bindToDefaultFactory<ScriptExecutor>()
          .bindToDefaultFactory<TerrainRenderer>()
          .bindToDefaultFactory<TerrainUtil>()
          .build();

  // Enter the game loop.
  std::cout << "Entering game loop." << std::endl;
  registry.get<Window>()->loop([&](float dt) {
    registry.get<EventHandler>()->onUpdate(dt);

    // Render the scene to a new frame.
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
    registry.get<TerrainRenderer>()->draw();
  });
}

}  // namespace tequila

int main() {
  try {
    tequila::run();
    return 0;
  } catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return 1;
  }
}
