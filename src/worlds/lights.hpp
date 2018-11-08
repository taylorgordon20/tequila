#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/data.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/spatial.hpp"
#include "src/common/voxels.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/voxels.hpp"

namespace tequila {

constexpr auto kMaxPositionLights = 4;

struct VertexLightData {
  float global_occlusion;
  std::array<glm::vec3, kMaxPositionLights> lights;
};

class VertexLightMap {
 public:
  VertexLightMap(size_t voxel_size) : size_(voxel_size) {}

  bool has(int x, int y, int z) {
    return map_.count(x + y * size_ + z * size_ * size_);
  }

  VertexLightData& get(int x, int y, int z) {
    return map_[x + y * size_ + z * size_ * size_];
  }

  VertexLightData& at(int x, int y, int z) {
    return map_.at(x + y * size_ + z * size_ * size_);
  }

 private:
  size_t size_;
  std::unordered_map<int, VertexLightData> map_;
};

// Maps a voxel array to light rays at each surface vertex.
struct VertexLights {
  auto operator()(ResourceDeps& deps, const std::string& voxel_key) {
    WORLD_TIMER(deps, "vertex_lights");

    auto voxels_util = registryGet<VoxelsUtil>(deps);
    auto voxels = deps.get<Voxels>(voxel_key);
    auto surface_vertices = deps.get<SurfaceVertices>(voxel_key);
    auto global_light = deps.get<WorldLight>();

    auto ret = std::make_shared<VertexLightMap>(voxels->size());
    for (auto [x, y, z] : *surface_vertices) {
      ret->get(x, y, z).global_occlusion = 1.0f;
      voxels_util->marchVoxels(
          voxels_util->getWorldCoords(*voxels, x, y, z),
          *global_light,
          32.0,
          [&](int vx, int vy, int vz) {
            return false;
            if (voxels_util->getVoxel(vx, vy, vz)) {
              ret->get(x, y, z).global_occlusion = 0.0f;
              return false;
            }
            return true;
          });
    }
    return ret;
  }
};

}  // namespace tequila