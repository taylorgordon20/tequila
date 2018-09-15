#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

#include "src/common/registry.hpp"

namespace tequila {

struct Camera {
  glm::vec3 position;
  glm::vec3 view;
  float fov;
  float aspect;
  float near;
  float far;

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