#include <Eigen/Dense>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <stdlib.h>
#include <ctime>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/meshes.hpp"
#include "src/common/shaders.hpp"
#include "src/common/voxels.hpp"
#include "src/common/window.hpp"

namespace tequila {

using namespace gl;

auto getCamera() {
  Camera camera;
  camera.position = glm::vec3(32.0f, 20.0f, -10.0f);
  camera.view = glm::vec3(0.0f, 0.0f, 1.0f);
  camera.fov = glm::radians(45.0f);
  camera.aspect = 4.0f / 3.0f;
  camera.near_distance = 0.1f;
  camera.far_distance = 100.0f;
  return camera;
}

auto getVoxelMesh() {
  return Table("octree_world").getObject<VoxelArray>("voxels/0").toMesh();
}

auto getShader() {
  return ShaderProgram(
      {makeVertexShader(loadFile("shaders/basic.vert.glsl")),
       makeFragmentShader(loadFile("shaders/basic.frag.glsl"))});
}

void run() {
  Application app;

  auto window = app.makeWindow(640, 480, "HelloTriangle", nullptr, nullptr);

  // Build globals.
  auto camera = getCamera();
  auto shader = getShader();
  auto mesh = getVoxelMesh();

  // Set window event callbacks.
  window->on<glfwSetKeyCallback>(
      [&](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
          window->close();
        } else if (key == GLFW_KEY_UP) {
          camera.position += 0.1f * camera.view;
        } else if (key == GLFW_KEY_DOWN) {
          camera.position -= 0.1f * camera.view;
        } else if (key == GLFW_KEY_LEFT) {
          camera.view = glm::rotateY(camera.view, 0.1f);
        } else if (key == GLFW_KEY_RIGHT) {
          camera.view = glm::rotateY(camera.view, -0.1f);
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
    glViewport(0, 0, w, h);
    if (height) {
      camera.aspect = static_cast<float>(width) / height;
    }
  });

  // Begin scene.
  window->loop([&](float dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.run([&] {
      shader.uniform("light", glm::normalize(glm::vec3(-2.0f, 4.0f, 1.0f)));
      shader.uniform("modelview_matrix", camera.viewMatrix());
      shader.uniform("normal_matrix", camera.normalMatrix());
      shader.uniform("projection_matrix", camera.projectionMatrix());
      mesh.draw(shader);
    });
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
