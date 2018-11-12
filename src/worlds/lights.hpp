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
  VertexLightMap(size_t voxel_size) : size_(voxel_size + 1) {}

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
    for (const auto& vertex : *surface_vertices) {
      auto x = std::get<0>(vertex);
      auto y = std::get<1>(vertex);
      auto z = std::get<2>(vertex);

      // Initialize occlusion to "not occluded".
      ret->get(x, y, z).global_occlusion = 1.0f;

      // Cast ray to detect if the the light to this vertex is occluded.
      auto dir = *global_light;
      auto from = voxels_util->getWorldCoords(*voxels, x, y, z);
      from += 0.01f * dir;
      voxels_util->marchVoxels(
          from, dir, 100.0, [&](int vx, int vy, int vz, float distance) {
            float cx = vx + 0.5f, cy = vy + 0.5f, cz = vz + 0.5f;
            if (sampler.getVoxel(cx, cy, cz)) {
              ret->get(x, y, z).global_occlusion = 0.15f;
              return false;
            }
            return true;
          });
    }
    return ret;
  }
};

}  // namespace tequila