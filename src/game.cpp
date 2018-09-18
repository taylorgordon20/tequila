#include <iostream>
#include <string>
#include <utility>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <cereal/archives/json.hpp>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/opengl.hpp"
#include "src/common/resources.hpp"
#include "src/worlds/core.hpp"
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
  std::cin >> world_name;

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
                       .withSingleton<WorldWindow>(window)
                       .build();

  // Set window event callbacks.
  window->on<glfwSetKeyCallback>(
      [&](int key, int scancode, int action, int mods) {
        using namespace gl;
        auto& camera = *resources.get<WorldCamera>();
        resources.invalidate<WorldCamera>();
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
          window->close();
        } else if (key == GLFW_KEY_UP) {
          camera.position += 0.1f * camera.view;
        } else if (key == GLFW_KEY_DOWN) {
          camera.position -= 0.1f * camera.view;
        } else if (key == GLFW_KEY_LEFT) {
          camera.view = glm::rotateY(camera.view, 0.5f);
        } else if (key == GLFW_KEY_RIGHT) {
          camera.view = glm::rotateY(camera.view, -0.5f);
        } else if (key == GLFW_KEY_PAGE_UP) {
          camera.position += 0.1f * glm::vec3(0.0f, 1.0f, 0.0f);
        } else if (key == GLFW_KEY_PAGE_DOWN) {
          camera.position -= 0.1f * glm::vec3(0.0f, 1.0f, 0.0f);
        } else if (key == GLFW_KEY_F) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else if (key == GLFW_KEY_G) {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
      });
  window->on<glfwSetWindowSizeCallback>([&](int width, int height) {
    int w, h;
    window->call<glfwGetFramebufferSize>(&w, &h);
    gl::glViewport(0, 0, w, h);
    if (height) {
      auto& camera = *resources.get<WorldCamera>();
      resources.invalidate<WorldCamera>();
      camera.aspect = static_cast<float>(width) / height;
    }
  });

  // Begin scene.
  std::cout << "Entering game loop." << std::endl;
  window->loop([&]() {
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
    TerrainRenderer::draw(resources);
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
