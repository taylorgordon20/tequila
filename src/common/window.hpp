#pragma once

#include <boost/any.hpp>

#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/opengl.hpp"

namespace tequila {

class Window {
 public:
  Window(GLFWwindow* window) : window_(window) {
    // Register callback function wrappers.
    glfwSetWindowUserPointer(window_, static_cast<void*>(this));
  }
  ~Window() {
    glfwDestroyWindow(window_);
  }

  template <auto callback>
  void clear() {
    callbacks_.erase((intptr_t)callback);
    callback(window_, nullptr);
  }

  template <auto callback, typename FunctionType>
  void on(FunctionType fn) {
    callbacks_[(intptr_t)callback] = std::move(fn);
    callback(window_, [](GLFWwindow * w, auto... args) -> auto {
      auto window = static_cast<Window*>(glfwGetWindowUserPointer(w));
      auto& cb = window->callbacks_[(intptr_t)callback];
      boost::any_cast<FunctionType>(cb)(args...);
    });
  }

  template <typename FunctionType>
  void loop(FunctionType fn) {
    static double previous_frame_time = glfwGetTime();
    while (!glfwWindowShouldClose(window_)) {
      float dt = static_cast<float>(glfwGetTime() - previous_frame_time);
      previous_frame_time = glfwGetTime();
      fn(dt);
      glfwSwapBuffers(window_);
      glfwPollEvents();
    }
  }

  template <auto glfw_fn, typename... Args>
  auto call(Args&&... args) {
    return glfw_fn(window_, std::forward<Args>(args)...);
  }

  void close() {
    glfwSetWindowShouldClose(window_, true);
  }

  bool inContext() {
    return glfwGetCurrentContext();
  }

 private:
  GLFWwindow* window_;
  std::unordered_map<intptr_t, boost::any> callbacks_;
};

class Application {
 public:
  Application() {
    ENFORCE(glfwInit());
    glfwSetErrorCallback([](int error, const char* cause) {
      std::cout << "error: " << error << " cause: " << cause << std::endl;
    });
  }
  ~Application() {
    glfwTerminate();
  }

  template <typename... GlfwArgs>
  std::shared_ptr<Window> makeWindow(GlfwArgs&&... args) {
    // Create a GLFW window.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    auto glfw_window = glfwCreateWindow(std::forward<GlfwArgs>(args)...);

    // Initialize OpenGL context with vsync and extension bindings.
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);
    initializeOpenGL();
    logInfoAboutOpenGL();

    // Return the window wrapper.
    return std::make_shared<Window>(glfw_window);
  }
};

}  // namespace tequila
