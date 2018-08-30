#pragma once

#include <iostream>

// Include GLFW which provided OpenGL context creation and window management.
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

// Include glbinding which provides bindings of all functions used in the
// version/profile of the current OpenGL context.
#include "glbinding/Binding.h"
#include "glbinding/ContextInfo.h"
#include "glbinding/Version.h"
#include "glbinding/gl/gl.h"

namespace tequila {

void print_opengl_information() {
  using glinfo = glbinding::ContextInfo;
  std::cout << "OpenGL Version: " << glinfo::version() << std::endl
            << "OpenGL Vendor: " << glinfo::vendor() << std::endl
            << "OpenGL Renderer: " << glinfo::renderer() << std::endl;
}

void initialize_bindings() {
  glbinding::Binding::initialize(false);  // Bindings are lazily initialized.
}

}  // namespace tequila