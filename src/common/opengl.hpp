#pragma once

#include <boost/format.hpp>
#include <cassert>
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

namespace tequila {

constexpr auto kOpenGLInfoTemplate = R"(
OpenGL Configuration
====================
version: %1%
vendor: %2%
renderer: %3%
====================
)";

void logInfoAboutOpenGL() {
  boost::format fmt(kOpenGLInfoTemplate);
  fmt % glbinding::ContextInfo::version().toString();
  fmt % glbinding::ContextInfo::vendor();
  fmt % glbinding::ContextInfo::renderer();
  std::cout << fmt;
}

void initializeBindingsForOpenGL() {
  // Bindings are lazily initialized.
  glbinding::Binding::initialize(false);
}

}  // namespace tequila