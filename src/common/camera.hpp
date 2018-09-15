#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

#include "src/common/registry.hpp"

namespace tequila {

struct Camera {
  glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 view = glm::vec3(0.0f, 0.0f, -1.0f);
  float fov = glm::radians(45.0f);
  float aspect = 1.0f;
  float near = 0.1f;
  float far = 100.0f;

  auto viewMatrix() {
    constexpr glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(position, position + view, up);
  }

  auto normalMatrix() {
    return glm::inverse(glm::transpose(glm::mat3(viewMatrix())));
  }

  auto projectionMatrix() {
    return glm::perspective(fov, aspect, near, far);
  }
};

}  // namespace tequila