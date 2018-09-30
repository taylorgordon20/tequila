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

class TextureBinding {
 public:
  TextureBinding(Texture& texture, int location);
  ~TextureBinding() noexcept;

  int location() const;

 private:
  Texture& texture_;
  int location_;
};

}  // namespace tequila
