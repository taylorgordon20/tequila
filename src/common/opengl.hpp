#pragma once

#include <iostream>

// Include GLFW which provided OpenGL context creation and window management.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Include glbinding which provides bindings of all functions used in the
// version/profile of the current OpenGL context.
#include <glbinding/Binding.h>
#include <glbinding/ContextInfo.h>
#include <glbinding/Version.h>
#include <glbinding/gl/gl.h>

#include "src/common/errors.hpp"

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
      glbinding::ContextInfo::version().toString(),
      glbinding::ContextInfo::vendor(),
      glbinding::ContextInfo::renderer());
}

inline void initializeBindingsForOpenGL() {
  // Bindings are lazily initialized.
  glbinding::Binding::initialize(false);
}

}  // namespace tequila