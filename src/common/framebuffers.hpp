#pragma once

#include "src/common/opengl.hpp"
#include "src/common/textures.hpp"

namespace tequila {

// TODO: Change texture pointers to std::shared_ptr.
class Framebuffer {
 public:
  Framebuffer(
      const std::tuple<int, int>& render_size,
      std::vector<std::shared_ptr<TextureOutput>> color_attachments,
      std::shared_ptr<TextureOutput> depth_attachment);
  ~Framebuffer();

  // Add explicit move constructor and assignment operator
  Framebuffer(Framebuffer&& other);
  Framebuffer& operator=(Framebuffer&& other);

  // Delete copy constructor and assignment operator
  Framebuffer(const Framebuffer&) = delete;
  Framebuffer& operator=(const Framebuffer&) = delete;

 private:
  std::vector<std::shared_ptr<TextureOutput>> color_attachments_;
  std::shared_ptr<TextureOutput> depth_attachment_;
  gl::GLuint framebuffer_;
  gl::GLuint depthbuffer_;

  template <typename FBO>
  friend class FramebufferBinding;
};

// TODO: Change texture pointers to std::shared_ptr.
class MultisampleFramebuffer {
 public:
  MultisampleFramebuffer(
      const std::tuple<int, int>& render_size,
      int render_samples,
      std::vector<std::shared_ptr<MultisampleTextureOutput>> color_attachments,
      std::shared_ptr<MultisampleTextureOutput> depth_attachment);
  ~MultisampleFramebuffer();

  // Add explicit move constructor and assignment operator
  MultisampleFramebuffer(MultisampleFramebuffer&& other);
  MultisampleFramebuffer& operator=(MultisampleFramebuffer&& other);

  // Delete copy constructor and assignment operator
  MultisampleFramebuffer(const MultisampleFramebuffer&) = delete;
  MultisampleFramebuffer& operator=(const MultisampleFramebuffer&) = delete;

 private:
  std::vector<std::shared_ptr<MultisampleTextureOutput>> color_attachments_;
  std::shared_ptr<MultisampleTextureOutput> depth_attachment_;
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
    std::shared_ptr<MultisampleTextureOutput> color_map) {
  using TexVec = std::vector<std::shared_ptr<MultisampleTextureOutput>>;
  return MultisampleFramebuffer(
      color_map->dimensions(),
      color_map->samples(),
      TexVec{color_map},
      nullptr);
}

inline Framebuffer makeFramebuffer(std::shared_ptr<TextureOutput> color_map) {
  using TexVec = std::vector<std::shared_ptr<TextureOutput>>;
  return Framebuffer(color_map->dimensions(), TexVec{color_map}, nullptr);
}

}  // namespace tequila