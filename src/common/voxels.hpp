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

}  // namespace tequila