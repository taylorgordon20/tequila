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
  void uniform(const std::string& name, const glm::vec3& value);
  void uniform(const std::string& name, const glm::vec4& value);
  void uniform(const std::string& name, const glm::mat3& value);
  void uniform(const std::string& name, const glm::mat4& value);

  // Methods to access uniform and attribute locations.
  int uniform(const std::string& name) const;
  int attribute(const std::string& name) const;

  void printDebugInfo() const;

 private:
  gl::GLint program_;
};

class ShaderManager {
 public:
  void loadSources(
      const std::string& name, const std::vector<ShaderSource>& sources);
  std::shared_ptr<ShaderProgram> get(const std::string& name) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shaders_;
};

template <>
inline std::shared_ptr<ShaderManager> gen(const Registry& registry) {
  return std::make_shared<ShaderManager>();
}

}  // namespace tequila