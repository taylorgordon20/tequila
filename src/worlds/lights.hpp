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
    StatsTimer timer(registryGet<Stats>(deps), "vertex_lights");

    auto voxels_util = registryGet<VoxelsUtil>(deps);
    auto voxels = deps.get<Voxels>(voxel_key);
    auto surface_vertices = deps.get<SurfaceVertices>(voxel_key);
    auto global_light = deps.get<WorldLight>();

    // Create a sampler to efficiently query voxel values.
    VoxelsSampler sampler(deps.get<WorldOctree>(), [&](int64_t cell) {
      auto voxel_keys = deps.get<VoxelKeys>(cell);
      ENFORCE(voxel_keys->size() == 1);
      return deps.get<Voxels>(voxel_keys->front());
    });

    auto ret = std::make_shared<VertexLightMap>(voxels->size());
    for (auto [x, y, z] : *surface_vertices) {
      ret->get(x, y, z).global_occlusion = 1.0f;
      auto dir = *global_light;
      auto from = voxels_util->getWorldCoords(*voxels, x, y, z);
      voxels_util->marchVoxels(from, dir, 100.0, [&](int vx, int vy, int vz) {
        float cx = vx + 0.5f, cy = vy + 0.5f, cz = vz + 0.5f;
        float proj = glm::dot(dir, glm::vec3(cx, cy, cz) - from);
        if (proj > 0.5f && sampler.getVoxel(cx, cy, cz)) {
          ret->get(x, y, z).global_occlusion = 0.2f;
          return false;
        }
        return true;
      });
    }
    return ret;
  }
};

}  // namespace tequila