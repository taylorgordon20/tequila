#pragma once

#include <cassert>
#include <unordered_map>

#include "src/common/opengl.hpp"

namespace tequila {

class Window {
 public:
  Window(GLFWwindow* window) : window_(window) {
    // Register callback function wrappers.
    glfwSetWindowUserPointer(window_, static_cast<void*>(this));
  }
  ~Window() { glfwDestroyWindow(window_); }

#define ON_IMPL(cb) on<decltype(cb), cb>(std::move(fn));
  // Keyboard input event.
  template <typename Fn>
  void onKey(Fn fn) {
    ON_IMPL(glfwSetKeyCallback);
  }

  // Window-resize input event.
  template <typename Fn>
  void onWindowSize(Fn fn) {
    ON_IMPL(glfwSetWindowSizeCallback);
  }

#undef ON_CB

  template <typename FunctionType>
  void loop(FunctionType fn) {
    while (!glfwWindowShouldClose(window_)) {
      fn();
      glfwSwapBuffers(window_);
      glfwPollEvents();
    }
  }

 protected:
  template <typename CallbackType, CallbackType cb, typename FunctionType>
  void on(FunctionType fn) {
    callbacks_[(intptr_t)cb] = std::move(fn);
    cb(window_, [](GLFWwindow * w, auto... args) -> auto {
      auto window = static_cast<Window*>(glfwGetWindowUserPointer(w));
      window->callbacks_[(intptr_t)cb]();
    });
  }

 private:
  GLFWwindow* window_;
  std::unordered_map<intptr_t, std::function<void()>> callbacks_;
};

class Application {
 public:
  Application() {
    assert(glfwInit());
    glfwSetErrorCallback([](int error, const char* cause) {
      std::cout << "error: " << error << " cause: " << cause << std::endl;
    });
  }
  ~Application() { glfwTerminate(); }

  template <typename... GlfwArgs>
  std::shared_ptr<Window> makeWindow(GlfwArgs&&... args) {
    // Create a GLFW window.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto glfw_window = glfwCreateWindow(std::forward<GlfwArgs>(args)...);

    // Initialize OpenGL context with vsync and extension bindings.
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);
    initializeBindingsForOpenGL();
    logInfoAboutOpenGL();

    // Return the window wrapper.
    return std::make_shared<Window>(glfw_window);
  }
};

}  // namespace tequila