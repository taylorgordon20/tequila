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

auto getVertexAmbientOcclusion(uint8_t occlusion_mask) {
  static std::vector<float> kCountToOcclusion{
      0.0f, 0.35f, 0.5f, 0.5f, 0.95f, 0.95f, 0.95f, 1.0f, 1.0f};
  static std::vector<int> kMaskToCount = [] {
    std::vector<int> ret(256);
    std::unordered_set<int> hits;
    std::vector<std::tuple<int, int, int>> stack;
    for (int mask = 0; mask < 256; mask += 1) {
      auto index = [](int x, int y, int z) {
        return (x % 2) + 2 * (y % 2) + 4 * (z % 2);
      };
      auto occluded = [&](int x, int y, int z) {
        return mask & (1 << index(x, y, z));
      };
      int count = 0;
      for (int x : {0, 1}) {
        for (int y : {0, 1}) {
          for (int z : {0, 1}) {
            hits.clear();
            stack.clear();
            stack.emplace_back(x, y, z);
            while (stack.size()) {
              auto [x, y, z] = stack.back();
              stack.pop_back();
              if (!occluded(x, y, z)) {
                hits.insert(index(x, y, z));
                if (!hits.count(index(x + 1, y, z))) {
                  stack.emplace_back(x + 1, y, z);
                }
                if (!hits.count(index(x, y + 1, z))) {
                  stack.emplace_back(x, y + 1, z);
                }
                if (!hits.count(index(x, y, z + 1))) {
                  stack.emplace_back(x, y, z + 1);
                }
              }
            }
            count = std::max<int>(count, hits.size());
          }
        }
      }
      ret.at(mask) = count;
    }
    return ret;
  }();
  return kCountToOcclusion.at(kMaskToCount.at(occlusion_mask));
}

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
  auto operator()(ResourceDeps& deps, int voxel_key) {
    StatsTimer timer(deps.get<WorldStats>(), "vertex_lights");

    auto global_light = deps.get<WorldLight>();

    // Load voxel data for this voxel array.
    auto surface_vertices = deps.get<SurfaceVertices>(voxel_key);
    auto voxel_config = deps.get<VoxelConfig>();
    auto [x0, y0, z0, x1, y1, z1] = voxel_config->voxelBox(voxel_key);

    // Create a sampler to efficiently query voxel values.
    VoxelAccessor accessor(deps); 
    auto ret = std::make_shared<VertexLightMap>(voxel_config->voxel_size);
    for (const auto& vertex : *surface_vertices) {
      auto ix = std::get<0>(vertex);
      auto iy = std::get<1>(vertex);
      auto iz = std::get<2>(vertex);

      // Initialize occlusion to "not occluded".
      auto& global_occlusion = ret->get(ix, iy, iz).global_occlusion;
      global_occlusion = 1.0f;

      // Initialize global vertex position and light direction.
      auto dir = *global_light;
      auto from = glm::vec3(x0 + ix, y0 + iy, z0 + iz) + 0.01f * dir;

      // Cast ray to detect if the the light to this vertex is occluded.
      marchVoxels(
          from,
          dir,
          100.0,
          [&](int x, int y, int z, float distance) {
            if (accessor.get(x, y, z)) {
              global_occlusion = 0.35f;
              return false;
            }
            return true;
          });

      // Also check to see if this vertex is a "corner".
      if (global_occlusion > 0.2f) {
        int x = x0 + ix, y = y0 + iy, z = z0 + iz;
        auto bit = [&](int ox, int oy, int oz) {
          auto value = accessor.get(x - 1 + ox, y - 1 + oy, z - 1 + oz);
          return value ? 1 : 0;
        };
        uint8_t mask = 0;
        mask += bit(0, 0, 0) << 0;
        mask += bit(1, 0, 0) << 1;
        mask += bit(0, 1, 0) << 2;
        mask += bit(1, 1, 0) << 3;
        mask += bit(0, 0, 1) << 4;
        mask += bit(1, 0, 1) << 5;
        mask += bit(0, 1, 1) << 6;
        mask += bit(1, 1, 1) << 7;
        auto ambient_occlusion = getVertexAmbientOcclusion(mask);
        global_occlusion = std::min<float>(global_occlusion, ambient_occlusion);
      }
    }
    return ret;
  }
};

}  // namespace tequila