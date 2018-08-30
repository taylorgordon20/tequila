#include <stdexcept>
#include "src/common/opengl.hpp"

namespace tequila {

void run() {
  GLFWwindow* window;

  if (!glfwInit()) {
    throw std::exception("Unable to initialize GLFW");
  }

  window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window) {
    glfwTerminate();
    throw std::exception("Unable to initialize GLFW window");
  }

  glfwMakeContextCurrent(window);

  // Initialize OpenGL context.
  initialize_bindings();
  print_opengl_information();

  while (!glfwWindowShouldClose(window)) {
    using namespace gl;

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.5f, 0.0f);
    glEnd();

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}