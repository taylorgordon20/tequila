#include "src/common/textures.hpp"

#include <boost/integer/integer_log2.hpp>

#include "src/common/images.hpp"
#include "src/common/opengl.hpp"

namespace tequila {

using namespace gl;

Texture::Texture(const ImageTensor& pixels) {
  ENFORCE(pixels.dimension(2) == 3 || pixels.dimension(2) == 4);

  // Create an OpenGL texture object.
  glGenTextures(1, &texture_);

  // Set the textures pixel data.
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      pixels.dimension(1),
      pixels.dimension(0),
      0,
      pixels.dimension(2) == 4 ? GL_RGBA : GL_RGB,
      GL_UNSIGNED_BYTE,
      pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
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

TextureOutput::TextureOutput(int width, int height, gl::GLenum format)
    : dimensions_(width, height), format_(format) {
  // Create an OpenGL texture object.
  glGenTextures(1, &texture_);

  // Set the textures pixel data.
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
  glBindTexture(GL_TEXTURE_2D, 0);
}

TextureOutput::~TextureOutput() {
  if (texture_) {
    glDeleteTextures(1, &texture_);
  }
}

TextureOutput::TextureOutput(TextureOutput&& other) : texture_(0) {
  *this = std::move(other);
}

TextureOutput& TextureOutput::operator=(TextureOutput&& other) {
  std::swap(texture_, other.texture_);
  return *this;
}

MultisampleTextureOutput::MultisampleTextureOutput(
    int width, int height, int samples, gl::GLenum format)
    : dimensions_(width, height), samples_(samples), format_(format) {
  // Create an OpenGL texture object.
  glGenTextures(1, &texture_);

  // Set the textures pixel data.
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture_);
  glTexImage2DMultisample(
      GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, true);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

MultisampleTextureOutput::~MultisampleTextureOutput() {
  if (texture_) {
    glDeleteTextures(1, &texture_);
  }
}

MultisampleTextureOutput::MultisampleTextureOutput(
    MultisampleTextureOutput&& other)
    : texture_(0) {
  *this = std::move(other);
}

MultisampleTextureOutput& MultisampleTextureOutput::operator=(
    MultisampleTextureOutput&& other) {
  std::swap(texture_, other.texture_);
  return *this;
}

TextureArray::TextureArray(const std::vector<ImageTensor>& pixels) {
  ENFORCE(pixels.size());
  size_t height = pixels.front().dimension(0);
  size_t width = pixels.front().dimension(1);
  size_t levels = boost::integer_log2(std::max(width, height));

  // Create texture object and allocate storage.
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
  glTexStorage3D(
      GL_TEXTURE_2D_ARRAY, levels, GL_RGBA8, width, height, pixels.size());

  // Set texture filter options.
  glTexParameteri(
      GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Set the texture pixel data.
  for (int i = 0; i < pixels.size(); i += 1) {
    ENFORCE(pixels.at(i).dimension(0) == height);
    ENFORCE(pixels.at(i).dimension(1) == width);
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        0,
        0,
        i,
        width,
        height,
        1,
        pixels.at(i).dimension(2) == 4 ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE,
        pixels.at(i).data());
  }

  // Generate mipmaps.
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

TextureArray::~TextureArray() {
  if (texture_) {
    glDeleteTextures(1, &texture_);
  }
}

TextureArray::TextureArray(TextureArray&& other) : texture_(0) {
  *this = std::move(other);
}

TextureArray& TextureArray::operator=(TextureArray&& other) {
  std::swap(texture_, other.texture_);
  return *this;
}

TextureCube::TextureCube(const std::vector<ImageTensor>& pixels) {
  ENFORCE(pixels.size() == 6);

  // Create texture object.
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_);

  // Set texture filter options.
  // TODO: Consider adding mipmapping by default.
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Set the texture pixel data.
  for (int i = 0; i < pixels.size(); i += 1) {
    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
        0,
        GL_RGB,
        pixels.at(i).dimension(1),
        pixels.at(i).dimension(0),
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        pixels.at(i).data());
  }

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

TextureCube::~TextureCube() {
  if (texture_) {
    glDeleteTextures(1, &texture_);
  }
}

TextureCube::TextureCube(TextureCube&& other) : texture_(0) {
  *this = std::move(other);
}

TextureCube& TextureCube::operator=(TextureCube&& other) {
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

TextureOutputBinding::TextureOutputBinding(TextureOutput& texture, int location)
    : texture_(texture), location_(location) {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D, texture_.texture_);
}

TextureOutputBinding::~TextureOutputBinding() noexcept {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int TextureOutputBinding::location() const {
  return location_;
}

MultisampleTextureOutputBinding::MultisampleTextureOutputBinding(
    MultisampleTextureOutput& texture, int location)
    : texture_(texture), location_(location) {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture_.texture_);
}

MultisampleTextureOutputBinding::~MultisampleTextureOutputBinding() noexcept {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

int MultisampleTextureOutputBinding::location() const {
  return location_;
}

TextureArrayBinding::TextureArrayBinding(TextureArray& texture, int location)
    : texture_(texture), location_(location) {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_.texture_);
}

TextureArrayBinding::~TextureArrayBinding() noexcept {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

int TextureArrayBinding::location() const {
  return location_;
}

TextureCubeBinding::TextureCubeBinding(TextureCube& texture, int location)
    : texture_(texture), location_(location) {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture_.texture_);
}

TextureCubeBinding::~TextureCubeBinding() noexcept {
  glActiveTexture(GL_TEXTURE0 + location_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

int TextureCubeBinding::location() const {
  return location_;
}

}  // namespace tequila