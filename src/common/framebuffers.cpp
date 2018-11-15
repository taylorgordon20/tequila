#include "src/common/framebuffers.hpp"

#include "src/common/opengl.hpp"

using namespace gl;

namespace tequila {

Framebuffer::Framebuffer(TextureOutput& color_texture) {
  // Create OpenGL framebuffer and depthbuffer objects.
  glGenFramebuffers(1, &framebuffer_);
  glGenRenderbuffers(1, &depthbuffer_);

  // Initialize the depthbuffer.
  auto [dw, dh] = color_texture.dimensions();
  auto samples = color_texture.samples();
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  glRenderbufferStorageMultisample(
      GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, dw, dh);

  // Initialize the framebuffer.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_);
  glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D_MULTISAMPLE,
      color_texture.id(),
      0);

  GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffers);
  ENFORCE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::Framebuffer(
    TextureOutput& color_texture, TextureOutput& depth_texture)
    : Framebuffer(color_texture) {
  // Attach texture to the depth buffer.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glFramebufferTexture(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture.id(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
  if (depthbuffer_) {
    glDeleteFramebuffers(1, &depthbuffer_);
  }
  if (framebuffer_) {
    glDeleteFramebuffers(1, &framebuffer_);
  }
}

Framebuffer::Framebuffer(Framebuffer&& other) {
  *this = std::move(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
  std::swap(framebuffer_, other.framebuffer_);
  std::swap(depthbuffer_, other.depthbuffer_);
  return *this;
}

FramebufferBinding::FramebufferBinding(Framebuffer& framebuffer)
    : framebuffer_(framebuffer) {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.framebuffer_);
}

FramebufferBinding::~FramebufferBinding() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace tequila