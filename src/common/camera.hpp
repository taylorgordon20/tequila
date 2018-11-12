#pragma once

#include <Eigen/Dense>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>

#include "src/common/spatial.hpp"

namespace tequila {

struct Camera {
  glm::vec3 position;
  glm::vec3 view;
  float fov;
  float aspect;
  float near_distance;
  float far_distance;

  Camera()
      : position(glm::vec3(0.0f, 0.0f, 0.0f)),
        view(glm::vec3(0.0f, 0.0f, 1.0f)),
        fov(glm::radians(45.0f)),
        aspect(1.0f),
        near_distance(0.1f),
        far_distance(100.0f) {}

  auto viewMatrix() const {
    return glm::lookAt(position, position + view, glm::vec3(0.0f, 1.0f, 0.0f));
  }

  auto normalMatrix() const {
    return glm::inverse(glm::transpose(glm::mat3(viewMatrix())));
  }

  auto projectionMatrix() const {
    return glm::perspective(fov, aspect, near_distance, far_distance);
  }
};

inline auto frustumVolume(const Camera& camera) {
  auto n = camera.near_distance;
  auto f = camera.far_distance;
  auto r = std::tanf(0.5 * camera.aspect);
  auto s = 4.0f / 3.0f * camera.aspect * r * r;
  return s * (f * f * f - n * n * n);
}

inline auto computeVisibleCells(const Camera& camera, const Octree& octree) {
  // Create an Eigen matrix mapping octree vertices to clip coordinates.
  Eigen::Matrix4f pv_mat;
  {
    auto view_proj = camera.projectionMatrix() * camera.viewMatrix();
    for (int col = 0; col < 4; col += 1) {
      for (int row = 0; row < 4; row += 1) {
        pv_mat(row, col) = view_proj[col][row];
      }
    }
  }

  // Create a matrix to use to store octree cell vertices.
  Eigen::Matrix<float, 4, 8> bb_mat;
  bb_mat.setOnes();
  {
    for (int z = 0; z < 2; z += 1) {
      for (int y = 0; y < 2; y += 1) {
        for (int x = 0; x < 2; x += 1) {
          bb_mat(0, x + y * 2 + z * 4) = x;
          bb_mat(1, x + y * 2 + z * 4) = y;
          bb_mat(2, x + y * 2 + z * 4) = z;
        }
      }
    }
  }

  // Recursively identify the minimum cell set visible to the camera.
  std::vector<int64_t> ret;
  octree.search([&](int64_t cell) {
    auto cell_box = octree.cellBox(cell);

    Eigen::Matrix4f tr_mat;
    tr_mat.setIdentity();
    tr_mat(0, 3) = std::get<0>(cell_box);
    tr_mat(1, 3) = std::get<1>(cell_box);
    tr_mat(2, 3) = std::get<2>(cell_box);
    tr_mat(0, 0) = std::get<3>(cell_box) - std::get<0>(cell_box);
    tr_mat(1, 1) = std::get<4>(cell_box) - std::get<1>(cell_box);
    tr_mat(2, 2) = std::get<5>(cell_box) - std::get<2>(cell_box);

    // Map the cell's bounding box matrix into clip coordinates
    Eigen::Array<float, 4, 8> clip = pv_mat * tr_mat * bb_mat;

    // We conclude that a cell does not intersect the frustum if all of the 8
    // bounding-box vertices are strictly outside one of the clip planes.
    if ((clip.row(0) < -clip.row(3)).all() ||
        (clip.row(0) > clip.row(3)).all() ||
        (clip.row(1) < -clip.row(3)).all() ||
        (clip.row(1) > clip.row(3)).all() ||
        (clip.row(2) < -clip.row(3)).all() ||
        (clip.row(2) > clip.row(3)).all()) {
      return false;
    }

    // Emit the cell and stop recursion if it's on the bottom-most level or if
    // the bounding box is almost entirely inside the frustum.
    constexpr auto softness = 1.2f;
    if ((octree.cellLevel(cell) + 1 >= octree.treeDepth()) ||
        ((clip.row(0) > -softness * clip.row(3)).all() &&
         (clip.row(0) < softness * clip.row(3)).all() &&
         (clip.row(1) > -softness * clip.row(3)).all() &&
         (clip.row(1) < softness * clip.row(3)).all() &&
         (clip.row(2) > -softness * clip.row(3)).all() &&
         (clip.row(2) < softness * clip.row(3)).all())) {
      ret.push_back(cell);
      return false;
    }

    return true;
  });

  return ret;
}

}  // namespace tequila