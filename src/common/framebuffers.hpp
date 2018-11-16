#pragma once

#include "src/common/opengl.hpp"
#include "src/common/textures.hpp"

namespace tequila {

class Framebuffer {
 public:
  Framebuffer(
      const std::tuple<int, int>& render_size,
      const std::vector<TextureOutput*>& color_attachments,
      TextureOutput* depth_attachment);
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

  template <typename FBO>
  friend class FramebufferBinding;
};

class MultisampleFramebuffer {
 public:
  MultisampleFramebuffer(
      const std::tuple<int, int>& render_size,
      int render_samples,
      const std::vector<MultisampleTextureOutput*>& color_attachments,
      MultisampleTextureOutput* depth_attachment);
  ~MultisampleFramebuffer();

  // Add explicit move constructor and assignment operator
  MultisampleFramebuffer(MultisampleFramebuffer&& other);
  MultisampleFramebuffer& operator=(MultisampleFramebuffer&& other);

  // Delete copy constructor and assignment operator
  MultisampleFramebuffer(const MultisampleFramebuffer&) = delete;
  MultisampleFramebuffer& operator=(const MultisampleFramebuffer&) = delete;

 private:
  gl::GLuint framebuffer_;
  gl::GLuint depthbuffer_;

  template <typename FBO>
  friend class FramebufferBinding;
};

template <typename FBO>
class FramebufferBinding {
 public:
  FramebufferBinding(FBO& fbo) : fbo_(fbo) {
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fbo_.framebuffer_);
  }
  ~FramebufferBinding() noexcept {
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
  }

 private:
  FBO& fbo_;
};

// Constructs a multi-sampled framebuffer with a single color attachment. This
// kind of FBO is useful for rendering a normal anti-aliased scene to texture.
inline MultisampleFramebuffer makeFramebuffer(
    MultisampleTextureOutput& color_map) {
  return MultisampleFramebuffer(
      color_map.dimensions(),
      color_map.samples(),
      std::vector<MultisampleTextureOutput*>{&color_map},
      nullptr);
}

inline Framebuffer makeFramebuffer(TextureOutput& color_map) {
  return Framebuffer(
      color_map.dimensions(), std::vector<TextureOutput*>{&color_map}, nullptr);
}

}  // namespace tequila