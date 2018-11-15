#pragma once

#include <iostream>

#ifdef _WIN32
#include "windows.h"
#endif

// Include GLFW which provided OpenGL context creation and window management.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Include glbinding which provides bindings of all functions used in the
// version/profile of the current OpenGL context.
#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/debug.h>
#include <glbinding-aux/types_to_string.h>
#include <glbinding/Binding.h>
#include <glbinding/Version.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include "src/common/errors.hpp"
#include "src/common/strings.hpp"

namespace tequila {

constexpr auto kOpenGLInfoTemplate = R"(
OpenGL Configuration
====================
version: %1%
vendor: %2%
renderer: %3%
====================
)";

inline void logInfoAboutOpenGL() {
  std::cout << format(
      kOpenGLInfoTemplate,
      glbinding::aux::ContextInfo::version().toString(),
      glbinding::aux::ContextInfo::vendor(),
      glbinding::aux::ContextInfo::renderer());
}

inline void initializeBindingsForOpenGL() {
  // Bindings are lazily initialized.
  glbinding::initialize(glfwGetProcAddress);
}

}  // namespace tequila