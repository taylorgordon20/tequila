#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "src/common/registry.hpp"

namespace tequila {

class ShaderProgram {
 public:
  ShaderProgram(
      const std::string& vert_shader_source,
      const std::string& frag_shader_source);

  // Executes the provide function in a scope with the shader bound.
  void run(std::function<void(ShaderProgram&)> fn);

  // Methods to specify uniform variables in shader.
  void uniform(const std::string& name, int value);
  void uniform(const std::string& name, const glm::vec3& value);
  void uniform(const std::string& name, const glm::vec4& value);
  void uniform(const std::string& name, const glm::mat4& value);

  // Methods to access attribute locations.
  int attribute(const std::string& name);
};

class ShaderManager {
 public:
  void loadSource(
      const std::string& name,
      const std::string& vert_source,
      const std::string& frag_source);

  void loadPaths(
      const std::string& name,
      const std::vector<std::string>& vert_paths,
      const std::vector<std::string>& frag_paths);

  ShaderProgram& get(const std::string& name) const;

 private:
  std::unordered_map<std::string, ShaderProgram> shaders_;
};

template <>
inline std::shared_ptr<ShaderManager> gen(const Registry& registry) {
  return std::make_shared<ShaderManager>();
}

}  // namespace tequila