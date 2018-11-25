#pragma once

#include <cereal/archives/binary.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "src/common/meshes.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

class VoxelArray {
 public:
  VoxelArray();

  // Methods to set voxels in local coordinates.
  void del(int x, int y, int z);
  void set(int x, int y, int z, uint32_t value);
  bool has(int x, int y, int z) const;
  uint32_t get(int x, int y, int z) const;

  // Methods to transform the voxels in world coordinates.
  void translate(float x, float y, float z);
  void rotate(float x, float y, float z, float angle);
  void scale(float x, float y, float z);

  // Returns the coordinates of each surface voxel.
  std::vector<std::tuple<int, int, int>> surfaceVoxels() const;
  std::vector<std::tuple<int, int, int>> surfaceVertices() const;

  size_t size() const;

  const glm::mat4& transform() const;
  Mesh toMesh() const;

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(
        voxels_,
        surface_voxels_,
        cereal::binary_data(glm::value_ptr(transform_), 4 * 4 * sizeof(float)));
  }

 private:
  void updateSurfaceVoxels(int x, int y, int z);

  CubeStore<uint32_t> voxels_;
  CubeStore<bool> surface_voxels_;
  glm::mat4 transform_;
};

// Convenience routine for marching over voxel coords intersecting a ray.
template <typename Function>
inline void marchVoxels(
    const glm::vec3& from,
    const glm::vec3& direction,
    float distance,
    Function voxel_fn) {
  // The starting position of the ray.
  auto x = from[0];
  auto y = from[1];
  auto z = from[2];

  // The signs of the ray direction vector components.
  auto sx = std::signbit(direction[0]);
  auto sy = std::signbit(direction[1]);
  auto sz = std::signbit(direction[2]);

  // The ray distance traveled per unit in each direction.
  glm::vec3 dir = glm::normalize(direction);
  auto dx = 1.0f / std::abs(dir[0]);
  auto dy = 1.0f / std::abs(dir[1]);
  auto dz = 1.0f / std::abs(dir[2]);

  // The ray distance to the next intersection in each direction.
  auto dist_x = (sx ? (x - std::floor(x)) : (1 + std::floor(x) - x)) * dx;
  auto dist_y = (sy ? (y - std::floor(y)) : (1 + std::floor(y) - y)) * dy;
  auto dist_z = (sz ? (z - std::floor(z)) : (1 + std::floor(z) - z)) * dz;

  // Advance voxel indices that intersect with the given ray.
  int ix = static_cast<int>(from[0]);
  int iy = static_cast<int>(from[1]);
  int iz = static_cast<int>(from[2]);
  for (auto march_distance = 0; march_distance < distance;) {
    if (!voxel_fn(ix, iy, iz, march_distance)) {
      break;
    }

    // Advance one voxel in the direction of nearest intersection.
    if (dist_x <= dist_y && dist_x <= dist_z) {
      march_distance = dist_x;
      ix += sx ? -1 : 1;
      dist_x += dx;
    } else if (dist_y <= dist_z) {
      march_distance = dist_y;
      iy += sy ? -1 : 1;
      dist_y += dy;
    } else {
      march_distance = dist_z;
      iz += sz ? -1 : 1;
      dist_z += dz;
    }
  }
}

}  // namespace tequila