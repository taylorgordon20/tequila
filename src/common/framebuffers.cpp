#include "src/common/framebuffers.hpp"

#include "src/common/errors.hpp"
#include "src/common/opengl.hpp"
#include "src/common/utils.hpp"

using namespace gl;

namespace tequila {

namespace {
auto getMaxColorAttachments() {
  GLint ret = 0;
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &ret);
  return ret;
}
}  // namespace

Framebuffer::Framebuffer(
    const std::tuple<int, int>& render_size,
    std::vector<std::shared_ptr<TextureOutput>> color_attachments,
    std::shared_ptr<TextureOutput> depth_attachment)
    : color_attachments_(std::move(color_attachments)),
      depth_attachment_(std::move(depth_attachment)) {
  ENFORCE(color_attachments_.size() <= getMaxColorAttachments());

  // Create OpenGL framebuffer and depthbuffer objects.
  glGenFramebuffers(1, &framebuffer_);
  glGenRenderbuffers(1, &depthbuffer_);

  // Bind the buffers.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  Finally finally([&] {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  });

  // Initialize the renderbuffers.
  glRenderbufferStorage(
      GL_RENDERBUFFER,
      GL_DEPTH_COMPONENT,
      std::get<0>(render_size),
      std::get<1>(render_size));
  glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_);

  // Attach color texture outputs.
  std::vector<GLenum> draw_buffers;
  for (int i = 0; i < color_attachments_.size(); i += 1) {
    ENFORCE(color_attachments_.at(i));
    draw_buffers.emplace_back(GL_COLOR_ATTACHMENT0 + i);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        draw_buffers.back(),
        GL_TEXTURE_2D,
        color_attachments_.at(i)->id(),
        0);
  }
  glDrawBuffers(draw_buffers.size(), draw_buffers.data());

  // Attach depth texture outputs.
  if (depth_attachment_) {
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depth_attachment_->id(),
        0);
  }

  // Make sure that everything worked before returning.
  ENFORCE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Framebuffer::~Framebuffer() {
  if (depthbuffer_) {
    glDeleteRenderbuffers(1, &depthbuffer_);
  }
  if (framebuffer_) {
    glDeleteFramebuffers(1, &framebuffer_);
  }
}

Framebuffer::Framebuffer(Framebuffer&& other)
    : framebuffer_(0), depthbuffer_(0) {
  *this = std::move(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
  std::swap(framebuffer_, other.framebuffer_);
  std::swap(depthbuffer_, other.depthbuffer_);
  return *this;
}

MultisampleFramebuffer::MultisampleFramebuffer(
    const std::tuple<int, int>& render_size,
    int render_samples,
    std::vector<std::shared_ptr<MultisampleTextureOutput>> color_attachments,
    std::shared_ptr<MultisampleTextureOutput> depth_attachment)
    : color_attachments_(std::move(color_attachments)),
      depth_attachment_(std::move(depth_attachment)) {
  ENFORCE(color_attachments_.size() <= getMaxColorAttachments());

  // Create OpenGL framebuffer and depthbuffer objects.
  glGenFramebuffers(1, &framebuffer_);
  glGenRenderbuffers(1, &depthbuffer_);

  // Bind the buffers.
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  Finally finally([&] {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  });

  // Initialize the renderbuffers.
  glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      render_samples,
      GL_DEPTH_COMPONENT,
      std::get<0>(render_size),
      std::get<1>(render_size));
  glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_);

  // Attach color texture outputs.
  std::vector<GLenum> draw_buffers;
  for (int i = 0; i < color_attachments_.size(); i += 1) {
    ENFORCE(color_attachments_.at(i));
    draw_buffers.emplace_back(GL_COLOR_ATTACHMENT0 + i);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        draw_buffers.back(),
        GL_TEXTURE_2D_MULTISAMPLE,
        color_attachments_.at(i)->id(),
        0);
  }
  glDrawBuffers(draw_buffers.size(), draw_buffers.data());

  // Attach depth texture outputs.
  if (depth_attachment_) {
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D_MULTISAMPLE,
        depth_attachment_->id(),
        0);
  }

  // Make sure that everything worked before returning.
  ENFORCE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

MultisampleFramebuffer::~MultisampleFramebuffer() {
  if (depthbuffer_) {
    glDeleteRenderbuffers(1, &depthbuffer_);
  }
  if (framebuffer_) {
    glDeleteFramebuffers(1, &framebuffer_);
  }
}

MultisampleFramebuffer::MultisampleFramebuffer(MultisampleFramebuffer&& other)
    : framebuffer_(0), depthbuffer_(0) {
  *this = std::move(other);
}

MultisampleFramebuffer& MultisampleFramebuffer::operator=(
    MultisampleFramebuffer&& other) {
  std::swap(framebuffer_, other.framebuffer_);
  std::swap(depthbuffer_, other.depthbuffer_);
  return *this;
}

}  // namespace tequila