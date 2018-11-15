#pragma once

#include "src/common/opengl.hpp"
#include "src/common/textures.hpp"

namespace tequila {

class Framebuffer {
 public:
  Framebuffer(TextureOutput& color_texture);
  Framebuffer(TextureOutput& color_texture, TextureOutput& depth_texture);
  ~Framebuffer();

  // Add explicit move constructor and assignment operator
  Framebuffer(Framebuffer&& other);
  Framebuffer& operator=(Framebuffer&& other);

  // Delete copy constructor and assignment operator
  Framebuffer(const Framebuffer&) = delete;
  Framebuffer& operator=(const Framebuffer&) = delete;

 private:
  gl::GLuint framebuffer_;
  gl::GLuint depthbuffer_;

  friend class FramebufferBinding;
};

class FramebufferBinding {
 public:
  FramebufferBinding(Framebuffer& framebuffer);
  ~FramebufferBinding() noexcept;

 private:
  Framebuffer& framebuffer_;
};

}  // namespace tequila