#include <iostream>
#include <string>
#include <thread>
#include <utility>

#ifdef _WIN32
#include "windows.h"
#endif

#include "src/common/errors.hpp"
#include "src/common/lua.hpp"
#include "src/common/opengl.hpp"
#include "src/common/resources.hpp"
#include "src/common/stats.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/events.hpp"
#include "src/worlds/opengl.hpp"
#include "src/worlds/scripts.hpp"
#include "src/worlds/sky.hpp"
#include "src/worlds/styles.hpp"
#include "src/worlds/terrain.hpp"
#include "src/worlds/ui.hpp"

namespace tequila {

auto getScriptContext() {
  auto ret = std::make_shared<LuaContext>();
  ret->state().script(loadFile("scripts/common.lua"));
  return ret;
}

auto getWorldCamera() {
  auto camera = std::make_shared<Camera>();
  camera->position = glm::vec3(50.0f, 50.0f, 50.0f);
  camera->view = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
  camera->fov = glm::radians(45.0f);
  camera->aspect = 4.0f / 3.0f;
  camera->near_distance = 0.1f;
  camera->far_distance = 256.0f;
  return camera;
}

auto getWorldLight() {
  return std::make_shared<glm::vec3>(
      glm::normalize(glm::vec3(-2.0f, 4.0f, 1.0f)));
}

auto getWorldUI() {
  return std::make_shared<UITree>();
}

void run() {
  // Figure out which world to load.
  std::string world_name;
  std::cout << "Enter world name (e.g. octree_world): ";
  std::getline(std::cin, world_name);
  if (world_name.empty()) {
    std::cout << "Defaulting to world 'octree_world'." << std::endl;
    world_name = "octree_world";
  }

  // Define a resource to store the static registry.
  auto static_context = std::make_shared<StaticContext>();

  // Define a factory to build the global asychronous task executor.
  auto executor_factory = [](const Registry& registry) {
    return std::make_shared<QueueExecutor>(20);
  };

  // Define a factory to build the world resources.
  auto resources_factory = [&](const Registry& registry) {
    return std::make_shared<Resources>(
        ResourcesBuilder()
            .withSeed<ScriptContext>(getScriptContext())
            .withSeed<WorldCamera>(getWorldCamera())
            .withSeed<WorldLight>(getWorldLight())
            .withSeed<WorldName>(world_name)
            .withSeed<WorldStaticContext>(static_context)
            .withSeed<WorldUI>(getWorldUI())
            .build());
  };

  // Define a factory to build the asynchronous interface to world resources.
  auto async_resources_factory = [world_name](const Registry& registry) {
    return std::make_shared<AsyncResources>(
        registry.get<Resources>(), registry.get<QueueExecutor>());
  };

  // Initialize game registry.
  Application app;
  Registry registry =
      RegistryBuilder()
          .bind<Window>(app.makeWindow(1024, 768, "Tequila!", nullptr, nullptr))
          .bind<AsyncResources>(async_resources_factory)
          .bind<QueueExecutor>(executor_factory)
          .bind<Resources>(resources_factory)
          .bind<Stats>(std::make_shared<Stats>())
          .bindToDefaultFactory<OpenGLContextExecutor>()
          .bindToDefaultFactory<EventHandler>()
          .bindToDefaultFactory<RectUIRenderer>()
          .bindToDefaultFactory<ScriptExecutor>()
          .bindToDefaultFactory<TerrainRenderer>()
          .bindToDefaultFactory<TextUIRenderer>()
          .bindToDefaultFactory<SkyRenderer>()
          .bindToDefaultFactory<StyleUIRenderer>()
          .bindToDefaultFactory<UIRenderer>()
          .bindToDefaultFactory<VoxelsUtil>()
          .build();

  // Update registry pointer inside resources.
  static_context->registry = &registry;

  // Enter the game loop.
  std::cout << "Entering game loop." << std::endl;
  registry.get<Window>()->loop([&](float dt) {
    // Handle the update game event.
    registry.get<EventHandler>()->update(dt);

    // Process OpenGL updates that are blocking async tasks.
    if (!registry.get<OpenGLContextExecutor>()->isEmpty()) {
      StatsTimer timer(registry.get<Stats>(), "process_gl_tasks");
      registry.get<OpenGLContextExecutor>()->process();
    }

    // Render the scene to a new frame.
    gl::glClearColor(0.62f, 0.66f, 0.8f, 0.0f);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
    registry.get<SkyRenderer>()->draw();
    registry.get<TerrainRenderer>()->draw();
    registry.get<UIRenderer>()->draw();
  });

  // Unblock and wait for any outstanding asynchronous tasks.
  registry.get<QueueExecutor>()->close();
  while (!registry.get<QueueExecutor>()->isDone()) {
    registry.get<OpenGLContextExecutor>()->process();
  }
  std::cout << "Shutting down!" << std::endl;
}

}  // namespace tequila

int main() {
  using namespace tequila;
  try {
    run();
    return 0;
  } catch (const std::exception& e) {
    LOG_ERROR(concat("Uncaught exception: ", e.what()));
    return 1;
  }
}
