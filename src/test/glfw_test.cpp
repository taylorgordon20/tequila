#include <stdlib.h>
#include <Eigen/Dense>
#include <iostream>
#include <vector>

#include "src/common/opengl.hpp"

namespace tequila {

using namespace gl;

auto kVertexShader = R"(
  #version 450
  in vec3 vp;
  void main() {
    gl_Position = vec4(vp, 1.0);
  }
)";

auto kFragmentShader = R"(
  #version 450
  out vec4 frag_color;
  void main() {
    frag_color = vec4(1.0, 0.0, 1.0, 1.0);
  }
)";

auto getTriangleGeometry() {
  Eigen::MatrixXf vertices(3, 3);
  vertices.col(0) << 0.0f, 0.5, 0.0f;
  vertices.col(1) << 0.5f, -0.5, 0.0f;
  vertices.col(2) << -0.5f, -0.5, 0.0f;
  return vertices;
}

auto getGeometryBuffer() {
  GLuint vbo, vao;
  glGenBuffers(1, &vbo);
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // Store the triangle positional attributes into the new VBO.
  auto vertices = getTriangleGeometry();
  glBufferData(
      GL_ARRAY_BUFFER,
      vertices.size() * sizeof(float),
      vertices.data(),
      GL_STATIC_DRAW);

  // Store the triangle geometry into the new VAO.
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return vao;
}

void checkShaderCompilation(GLuint shader) {
  GLint compile_status = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
  if (compile_status == 0) {
    std::cout << "Error compiling shader: " << shader << std::endl;

    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_log(log_length);
    glGetShaderInfoLog(shader, log_length, &log_length, error_log.data());
    std::cout << &error_log[0];

    exit(EXIT_FAILURE);
  }
}

auto buildShader() {
  // Compile the vertex shader.
  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &kVertexShader, NULL);
  glCompileShader(vs);
  checkShaderCompilation(vs);

  // Compile the fragment shader.
  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &kFragmentShader, NULL);
  glCompileShader(fs);
  checkShaderCompilation(fs);

  // Build the shader program.
  auto shader = glCreateProgram();
  glAttachShader(shader, vs);
  glAttachShader(shader, fs);
  glLinkProgram(shader);

  return shader;
}

void errorCallback(int error, const char* cause) {
  std::cout << "Error: " << error << " Cause: " << cause << std::endl;
}

void keyCallback(
    GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

void sizeCallback(GLFWwindow* window, int width, int height) {
  // Get the size of the windows frame.
  int w, h;
  glfwGetFramebufferSize(window, &w, &h);
  glViewport(0, 0, w, h);
}

class Scene {
 public:
  Scene(GLuint shader, GLuint vao) : shader_(shader), vao_(vao) {}

  void draw() {
    // Clear the frame buffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the triangle.
    glUseProgram(shader_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
  }

 private:
  GLuint shader_;
  GLuint vao_;
};

void run() {
  // Initialize GLFW.
  glfwSetErrorCallback(errorCallback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  // Force OpenGL version to 4.1 core profile.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a window with an OpenGL context. This will fail if the requested
  // OpenGL profile version is not supported/available on this system.
  GLFWwindow* window = glfwCreateWindow(640, 480, "HelloTriangle", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  // Initialize the OpenGL window.
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Set window event callbacks.
  glfwSetKeyCallback(window, keyCallback);
  glfwSetWindowSizeCallback(window, sizeCallback);

  // Initialize OpenGL function bindings.
  initializeBindingsForOpenGL();
  logInfoAboutOpenGL();

  // Handle window events and draw our scene.
  Scene scene(buildShader(), getGeometryBuffer());
  while (!glfwWindowShouldClose(window)) {
    scene.draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Kill the window and shutdown GLFW.
  glfwDestroyWindow(window);
  glfwTerminate();
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}
