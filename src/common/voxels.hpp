#pragma once

#include <cereal/archives/binary.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <unordered_map>

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

  const glm::mat4& transform() const;
  Mesh toMesh() const;

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(
        voxels_,
        cereal::binary_data(glm::value_ptr(transform_), 4 * 4 * sizeof(float)));
  }

 private:
  CubeStore<uint32_t> voxels_;
  glm::mat4 transform_;
};

}  // namespace tequila