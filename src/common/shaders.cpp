#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/errors.hpp"
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
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    std::vector<GLchar> error_log(log_length);
    glGetShaderInfoLog(shader, log_length, &log_length, error_log.data());
    throwError("Shader compilation error: %1%", &error_log[0]);
  }
}
}  // anonymous namespace

ShaderProgram::ShaderProgram(const std::vector<ShaderSource>& sources) {
  std::vector<GLint> shaders;
  for (const auto& source : sources) {
    const char* code = source.code.c_str();
    shaders.push_back(gl::glCreateShader(source.kind));
    glShaderSource(shaders.back(), 1, &code, NULL);
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

ShaderProgram::ShaderProgram(ShaderProgram&& other) : program_(0) {
  *this = std::move(other);
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) {
  std::swap(program_, other.program_);
  return *this;
}

void ShaderProgram::run(std::function<void()> fn) {
  try {
    glUseProgram(program_);
    fn();
  } catch (...) {
    glUseProgram(0);
    throw;
  }
}

void ShaderProgram::uniform(const std::string& name, bool value) {
  glUniform1b(uniform(name), value);
}

void ShaderProgram::uniform(const std::string& name, int value) {
  glUniform1i(uniform(name), value);
}

void ShaderProgram::uniform(const std::string& name, float value) {
  glUniform1f(uniform(name), value);
}

void ShaderProgram::uniform(const std::string& name, const glm::vec2& value) {
  glUniform2fv(uniform(name), 1, glm::value_ptr(value));
}

void ShaderProgram::uniform(const std::string& name, const glm::vec3& value) {
  glUniform3fv(uniform(name), 1, glm::value_ptr(value));
}

void ShaderProgram::uniform(const std::string& name, const glm::vec4& value) {
  glUniform4fv(uniform(name), 1, glm::value_ptr(value));
}

void ShaderProgram::uniform(const std::string& name, const glm::mat3& value) {
  glUniformMatrix3fv(uniform(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::uniform(const std::string& name, const glm::mat4& value) {
  glUniformMatrix4fv(uniform(name), 1, GL_FALSE, glm::value_ptr(value));
}

int ShaderProgram::uniform(const std::string& name) const {
  auto ret = glGetUniformLocation(program_, name.c_str());
  if (ret == -1) {
    throwError("Invalid shader uniform: %1%", name);
  }
  return ret;
}

int ShaderProgram::attribute(const std::string& name) const {
  auto ret = glGetAttribLocation(program_, name.c_str());
  if (ret == -1) {
    throwError("Invalid shader attribute: %1%", name);
  }
  return ret;
}

bool ShaderProgram::hasUniform(const std::string& name) const {
  return glGetUniformLocation(program_, name.c_str()) != -1;
}

bool ShaderProgram::hasAttribute(const std::string& name) const {
  return glGetAttribLocation(program_, name.c_str()) != -1;
}

void ShaderProgram::printDebugInfo() const {
  constexpr auto kBufferSize = 256;
  GLint size, count;
  GLenum type;
  GLsizei name_size;
  GLchar name[kBufferSize];

  // Print all of the uniforms.
  glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &count);
  for (GLint i = 0; i < count; i += 1) {
    glGetActiveUniform(
        program_, i, kBufferSize, &name_size, &size, &type, name);
    std::cout << format("uniform(%1%) = %2%[%3%]", i, type, size) << std::endl;
  }
}

}  // namespace tequila