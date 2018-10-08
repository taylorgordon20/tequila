#pragma once

#include "src/common/images.hpp"
#include "src/common/opengl.hpp"

namespace tequila {

class Texture {
 public:
  Texture(const ImageTensor& pixels);
  ~Texture();

  // Add explicit move constructor and assignment operator
  Texture(Texture&& other);
  Texture& operator=(Texture&& other);

  // Delete copy constructor and assignment operator
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

 private:
  gl::GLuint texture_;

  friend class TextureBinding;
};

class TextureArray {
 public:
  TextureArray(const std::vector<ImageTensor>& pixels);
  ~TextureArray();

  // Add explicit move constructor and assignment operator
  TextureArray(TextureArray&& other);
  TextureArray& operator=(TextureArray&& other);

  // Delete copy constructor and assignment operator
  TextureArray(const TextureArray&) = delete;
  TextureArray& operator=(const TextureArray&) = delete;

 private:
  gl::GLuint texture_;

  friend class TextureArrayBinding;
};

class TextureBinding {
 public:
  TextureBinding(Texture& texture, int location);
  ~TextureBinding() noexcept;

  int location() const;

 private:
  Texture& texture_;
  int location_;
};

class TextureArrayBinding {
 public:
  TextureArrayBinding(TextureArray& texture, int location);
  ~TextureArrayBinding() noexcept;

  int location() const;

 private:
  TextureArray& texture_;
  int location_;
};

}  // namespace tequila
