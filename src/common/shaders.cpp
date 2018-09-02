#include <boost/scope_exit.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/opengl.hpp"
#include "src/common/shaders.hpp"

namespace tequila {

using namespace gl;

namespace {
void checkShaderCompilation(GLuint shader) {
  GLint compilation_status = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compilation_status);
  if (!compilation_status) {
    GLint log_length = 0;
    std::vector<GLchar> error_log(log_length);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    glGetShaderInfoLog(shader, log_length, &log_length, error_log.data());

    boost::format error("Shader compilation error: %1%");
    error % &error_log[0];
    throw std::runtime_error(error.str().c_str());
  }
}
}  // anonymous namespace

ShaderProgram::ShaderProgram(const std::vector<ShaderSource>& sources) {
  std::vector<GLint> shaders;
  for (const auto& pair : sources) {
    shaders.push_back(gl::glCreateShader(pair.first));
    glShaderSource(shaders.back(), 1, &pair.second, NULL);
    glCompileShader(shaders.back());
    checkShaderCompilation(shaders.back());
  }

  // Build the aggregate shader program.
  program_ = glCreateProgram();
  for (const auto shader : shaders) {
    glAttachShader(program_, shader);
  }
  glLinkProgram(program_);
}

ShaderProgram::~ShaderProgram() {
  glDeleteProgram(program_);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) {
  *this = std::move(other);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) {
  program_ = other.program_;
  other.program_ = 0;
  return *this;
}

void ShaderProgram::run(std::function<void()> fn) {
  BOOST_SCOPE_EXIT_ALL(&) {
    glUseProgram(0);
  };
  glUseProgram(program_);
  fn();
}

void ShaderProgram::uniform(const std::string& name, int value) {}

void ShaderProgram::uniform(const std::string& name, const glm::vec3& value) {}

void ShaderProgram::uniform(const std::string& name, const glm::vec4& value) {}

void ShaderProgram::uniform(const std::string& name, const glm::mat4& value) {}

int ShaderProgram::attribute(const std::string& name) {
  return glGetAttribLocation(program_, name.c_str());
}

}  // namespace tequila