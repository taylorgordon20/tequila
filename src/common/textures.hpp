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

  static Texture makeEmpty(int width, int height, int depth);

 private:
  gl::GLuint texture_;
  std::tuple<int64_t, int64_t> dimensions_;

  friend class TextureBinding;
};

class TextureOutput {
 public:
  TextureOutput(int width, int height, gl::GLenum format = gl::GL_RGBA8);
  ~TextureOutput();

  // Add explicit move constructor and assignment operator
  TextureOutput(TextureOutput&& other);
  TextureOutput& operator=(TextureOutput&& other);

  // Delete copy constructor and assignment operator
  TextureOutput(const TextureOutput&) = delete;
  TextureOutput& operator=(const TextureOutput&) = delete;

  auto dimensions() const {
    return dimensions_;
  }

  auto format() const {
    return format_;
  }

  auto id() const {
    return texture_;
  }

 private:
  gl::GLuint texture_;
  std::tuple<int, int> dimensions_;
  gl::GLenum format_;

  friend class TextureOutputBinding;
};

class MultisampleTextureOutput {
 public:
  MultisampleTextureOutput(
      int width, int height, int samples, gl::GLenum format = gl::GL_RGBA8);
  ~MultisampleTextureOutput();

  // Add explicit move constructor and assignment operator
  MultisampleTextureOutput(MultisampleTextureOutput&& other);
  MultisampleTextureOutput& operator=(MultisampleTextureOutput&& other);

  // Delete copy constructor and assignment operator
  MultisampleTextureOutput(const MultisampleTextureOutput&) = delete;
  MultisampleTextureOutput& operator=(const MultisampleTextureOutput&) = delete;

  auto dimensions() const {
    return dimensions_;
  }

  auto samples() const {
    return samples_;
  }

  auto format() const {
    return format_;
  }

  auto id() const {
    return texture_;
  }

 private:
  gl::GLuint texture_;
  std::tuple<int, int> dimensions_;
  int samples_;
  gl::GLenum format_;

  friend class MultisampleTextureOutputBinding;
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

class TextureCube {
 public:
  TextureCube(const std::vector<ImageTensor>& pixels);
  ~TextureCube();

  // Add explicit move constructor and assignment operator
  TextureCube(TextureCube&& other);
  TextureCube& operator=(TextureCube&& other);

  // Delete copy constructor and assignment operator
  TextureCube(const TextureCube&) = delete;
  TextureCube& operator=(const TextureCube&) = delete;

 private:
  gl::GLuint texture_;

  friend class TextureCubeBinding;
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

class TextureOutputBinding {
 public:
  TextureOutputBinding(TextureOutput& texture, int location);
  ~TextureOutputBinding() noexcept;

  int location() const;

 private:
  TextureOutput& texture_;
  int location_;
};

class MultisampleTextureOutputBinding {
 public:
  MultisampleTextureOutputBinding(
      MultisampleTextureOutput& texture, int location);
  ~MultisampleTextureOutputBinding() noexcept;

  int location() const;

 private:
  MultisampleTextureOutput& texture_;
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

class TextureCubeBinding {
 public:
  TextureCubeBinding(TextureCube& texture, int location);
  ~TextureCubeBinding() noexcept;

  int location() const;

 private:
  TextureCube& texture_;
  int location_;
};

}  // namespace tequila
