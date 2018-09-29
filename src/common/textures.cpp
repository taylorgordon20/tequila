#include "src/common/textures.hpp"

#include "src/common/images.hpp"
#include "src/common/opengl.hpp"

namespace tequila {

using namespace gl;

Texture::Texture(const ImageTensor& pixels) {
  // Create an OpenGL texture object.
  glGenTextures(1, &texture_);

  // Set the textures pixel data.
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGB,
      pixels.dimension(1),
      pixels.dimension(0),
      0,
      GL_RGB,
      GL_UNSIGNED_BYTE,
      pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
  if (texture_) {
    glDeleteTextures(1, &texture_);
  }
}

Texture::Texture(Texture&& other) : texture_(0) {
  *this = std::move(other);
}

Texture& Texture::operator=(Texture&& other) {
  std::swap(texture_, other.texture_);
  return *this;
}

TextureBinding::TextureBinding(Texture& texture, int location)
    : texture_(texture), location_(location) {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D, texture_.texture_);
}

TextureBinding::~TextureBinding() noexcept {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int TextureBinding::location() const {
  return location_;
}

}  // namespace tequila