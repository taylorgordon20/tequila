#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>

#include "src/common/opengl.hpp"
#include "src/common/registry.hpp"

namespace tequila {

struct ShaderSource {
  gl::GLenum kind;
  std::string code;
  ShaderSource(gl::GLenum kind, std::string code)
      : kind(kind), code(std::move(code)) {}
};

template <typename StringType>
inline auto makeVertexShader(StringType&& code) {
  return ShaderSource(gl::GL_VERTEX_SHADER, std::forward<StringType>(code));
}

template <typename StringType>
inline auto makeFragmentShader(StringType&& code) {
  return ShaderSource(gl::GL_FRAGMENT_SHADER, std::forward<StringType>(code));
}

class ShaderProgram {
 public:
  ShaderProgram(const std::vector<ShaderSource>& sources);
  ~ShaderProgram();

  // Add explicit move constructor and assignment operator
  ShaderProgram(ShaderProgram&& other);
  ShaderProgram& operator=(ShaderProgram&& other);

  // Delete copy constructor and assignment operator
  ShaderProgram(const ShaderProgram&) = delete;
  ShaderProgram& operator=(const ShaderProgram&) = delete;

  // Executes the provide function in a scope with the shader bound.
  void run(std::function<void()> fn);

  // Methods to specify uniform variables in shader.
  void uniform(const std::string& name, int value);
  void uniform(const std::string& name, float value);
  void uniform(const std::string& name, const glm::vec2& value);
  void uniform(const std::string& name, const glm::vec3& value);
  void uniform(const std::string& name, const glm::vec4& value);
  void uniform(const std::string& name, const glm::mat3& value);
  void uniform(const std::string& name, const glm::mat4& value);

  // Methods to access uniform and attribute locations.
  int uniform(const std::string& name) const;
  int attribute(const std::string& name) const;

  // Methods to conditionally check for shader state.
  bool hasUniform(const std::string& name) const;
  bool hasAttribute(const std::string& name) const;

  void printDebugInfo() const;

 private:
  gl::GLint program_;
};

}  // namespace tequila