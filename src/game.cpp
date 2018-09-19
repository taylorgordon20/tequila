#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <utility>

#include "src/common/camera.hpp"
#include "src/common/errors.hpp"
#include "src/common/opengl.hpp"
#include "src/common/resources.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/input.hpp"
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

void run() {
  // Figure out what world to load.
  std::string world_name;
  std::cout << "Enter world name (e.g. octree_world): ";
  std::getline(std::cin, world_name);
  if (world_name.empty()) {
    std::cout << "Defaulting to world 'octree_world'." << std::endl;
    world_name = "octree_world";
  }

  // Initialize game singletons (e.g. camera, window)
  Application app;
  auto window = app.makeWindow(1024, 768, "Tequila!", nullptr, nullptr);
  auto world_camera = makeWorldCamera();
  auto world_light = makeWorldLight();

  // Prepare the game resources.
  std::cout << "Preparing resources for " << world_name << "..." << std::endl;
  auto resources = ResourcesBuilder()
                       .withSingleton<WorldCamera>(world_camera)
                       .withSingleton<WorldLight>(world_light)
                       .withSingleton<WorldName>(world_name)
                       .build();

  // Play the game.
  // clang-format off
  std::cout << "Entering game loop." << std::endl;
  window->loop([
    &resources,
    world_handler = WorldHandler(window, resources),
    terrain_renderer = TerrainRenderer(resources),
    terrain_handler = TerrainHandler(window, resources)
  ](float dt) mutable {
    // Update game state by processing any event changes.
    world_handler.update(dt);
    terrain_handler.update(dt);

    // Render the scene to a new frame.
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
    terrain_renderer.draw();
  });
  // clang-format on
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
