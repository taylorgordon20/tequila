#include <stdlib.h>
#include <Eigen/Dense>
#include <iostream>
#include <utility>
#include <vector>

#include "src/common/meshes.hpp"
#include "src/common/shaders.hpp"
#include "src/common/window.hpp"

namespace tequila {

using namespace gl;

auto getMesh() {
  Eigen::Matrix<float, 3, Eigen::Dynamic> vertices(3, 3);
  vertices.col(0) << 0.0f, 0.5f, 0.0f;
  vertices.col(1) << 0.5f, -0.5f, 0.0f;
  vertices.col(2) << -0.5f, -0.5f, 0.0f;
  return MeshBuilder().setPositions(std::move(vertices)).build();
}

auto getShader() {
  constexpr std::pair<GLenum, const char*> kVertexShader(
      GL_VERTEX_SHADER,
      R"(
      #version 410
      in vec3 position;
      void main() {
        gl_Position = vec4(position, 1.0);
      }
      )");

  constexpr std::pair<GLenum, const char*> kFragmentShader(
      GL_FRAGMENT_SHADER,
      R"(
      #version 410
      out vec4 frag_color;
      void main() {
        frag_color = vec4(1.0, 0.0, 1.0, 1.0);
      }
      )");
  return ShaderProgram({kVertexShader, kFragmentShader});
}

void run() {
  Application app;

  auto window = app.makeWindow(640, 480, "HelloTriangle", nullptr, nullptr);

  // Set window event callbacks.
  window->on<glfwSetKeyCallback>(
      [&](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
          window->close();
        }
      });
  window->on<glfwSetWindowSizeCallback>(
      [&](int width, int height) { glViewport(0, 0, width, height); });

  // Begin scene.
  window->loop([mesh = getMesh(), shader = getShader()]() mutable {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.run([&] { mesh.draw(shader); });
  });
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}
