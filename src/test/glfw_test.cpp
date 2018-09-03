#include <Eigen/Dense>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <utility>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/errors.hpp"
#include "src/common/meshes.hpp"
#include "src/common/shaders.hpp"
#include "src/common/window.hpp"

namespace tequila {

using namespace gl;

auto loadFile(std::string relative_path) {
  auto path = boost::filesystem::system_complete(relative_path);
  if (!boost::filesystem::exists(path)) {
    path = boost::filesystem::system_complete(relative_path.insert(0, "../"));
  }
  assert(boost::filesystem::exists(path));
  boost::filesystem::ifstream ifs(path);
  return std::string(
      (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

auto getCamera() {
  Camera camera;
  camera.position[2] = 2.0f;
  camera.view[2] = -1.0f;
  camera.fov = glm::radians(45.0f);
  camera.aspect = 4.0f / 3.0f;
  camera.near = 0.1f;
  camera.far = 100.0f;
  return camera;
}

auto getMesh() {
  // Set vertex position.
  Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 9);
  positions.col(0) << 0.0f, 0.5f, 0.0f;
  positions.col(1) << -0.5f, -0.5f, 0.5f;
  positions.col(2) << 0.5f, -0.5f, 0.5f;
  positions.col(3) << 0.0f, 0.5f, 0.0f;
  positions.col(4) << 0.5f, -0.5f, 0.5f;
  positions.col(5) << 0.0f, -0.5f, -0.5f;
  positions.col(6) << 0.0f, 0.5f, 0.0f;
  positions.col(7) << 0.0f, -0.5f, -0.5f;
  positions.col(8) << -0.5f, -0.5f, 0.5f;

  // Set vertex normals.
  constexpr auto sqrt2i = 0.70710678118f;
  constexpr auto sqrt3i = 0.57735026919f;
  Eigen::Matrix<float, 3, Eigen::Dynamic> normals(3, 9);
  normals.col(0) << 0.0f, sqrt2i, sqrt2i;
  normals.col(1) << 0.0f, sqrt2i, sqrt2i;
  normals.col(2) << 0.0f, sqrt2i, sqrt2i;
  normals.col(3) << sqrt3i, sqrt3i, -sqrt3i;
  normals.col(4) << sqrt3i, sqrt3i, -sqrt3i;
  normals.col(5) << sqrt3i, sqrt3i, -sqrt3i;
  normals.col(6) << -sqrt3i, sqrt3i, -sqrt3i;
  normals.col(7) << -sqrt3i, sqrt3i, -sqrt3i;
  normals.col(8) << -sqrt3i, sqrt3i, -sqrt3i;

  // Set vertex colors.
  Eigen::Matrix<float, 3, Eigen::Dynamic> colors(3, 9);
  colors.col(0) << 1.0f, 0.0f, 0.0f;
  colors.col(1) << 0.0f, 1.0f, 0.0f;
  colors.col(2) << 0.0f, 0.0f, 1.0f;
  colors.col(3) << 1.0f, 0.0f, 0.0f;
  colors.col(4) << 0.0f, 0.0f, 1.0f;
  colors.col(5) << 1.0f, 1.0f, 1.0f;
  colors.col(6) << 1.0f, 0.0f, 0.0f;
  colors.col(7) << 1.0f, 1.0f, 1.0f;
  colors.col(8) << 0.0f, 1.0f, 0.0f;

  return MeshBuilder()
      .setPositions(std::move(positions))
      .setNormals(std::move(normals))
      .setColors(std::move(colors))
      .build();
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
  auto mesh = getMesh();
  std::cout << "4" << std::endl;

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
        }
      });
  window->on<glfwSetWindowSizeCallback>(
      [&](int width, int height) { glViewport(0, 0, width, height); });

  // Begin scene.
  window->loop([&]() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.run([&] {
      shader.uniform("light", glm::normalize(glm::vec3(-1.0f, 1.0f, 1.0f)));
      shader.uniform("view_matrix", camera.viewMatrix());
      shader.uniform("normal_matrix", camera.normalMatrix());
      shader.uniform("projection_matrix", camera.projectionMatrix());
      mesh.draw(shader);
    });

    // Poor-man's frame counter.
    THROTTLED_FN(1.0, [&](int64_t calls, int64_t ticks) {
      std::string indicator(1 + calls % 3, '.');
      std::cout << "\rFPS: " << ticks << indicator << "  ";
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
