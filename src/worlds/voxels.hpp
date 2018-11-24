#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/data.hpp"
#include "src/common/maps.hpp"
#include "src/common/resources.hpp"
#include "src/common/spatial.hpp"
#include "src/common/stats.hpp"
#include "src/common/timers.hpp"
#include "src/common/voxels.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

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

// Specifies how voxel arrays are layed out in the world.
struct VoxelConfigData {
  int voxel_size;
  int grid_size;

  VoxelConfigData(int voxel_size, int grid_size)
      : voxel_size(voxel_size), grid_size(grid_size) {}

  auto voxelKey(int x, int y, int z) {
    int vx = x / voxel_size;
    int vy = y / voxel_size;
    int vz = z / voxel_size;
    return vx + vy * grid_size + vz * grid_size * grid_size;
  }

  auto voxelBox(int voxel_key) {
    auto vx = voxel_key % grid_size;
    auto vy = (voxel_key / grid_size) % grid_size;
    auto vz = voxel_key / grid_size / grid_size;
    auto x0 = voxel_size * vx, x1 = voxel_size * (vx + 1);
    auto y0 = voxel_size * vy, y1 = voxel_size * (vy + 1);
    auto z0 = voxel_size * vz, z1 = voxel_size * (vz + 1);
    return std::tuple(x0, y0, z0, x1, y1, z1);
  }
};

// Maps the voxel config for the current world.
// TODO: Make this config part of the WorldDB.
struct VoxelConfig {
  auto operator()(ResourceDeps& deps) {
    constexpr int kVoxelArraySize = 64;
    auto octree = deps.get<WorldOctree>();
    return std::make_shared<VoxelConfigData>(
        kVoxelArraySize,
        static_cast<int>(octree->size()) / kVoxelArraySize);
  }
};

struct Voxels {
  auto operator()(ResourceDeps& deps, int voxel_key) {
    auto world_db = deps.get<WorldTable>();
    return std::make_shared<VoxelArray>(
        world_db->getObject<VoxelArray>(format("voxels/%1%", voxel_key)));
  }
};

struct SurfaceVoxels {
  auto operator()(ResourceDeps& deps, int voxel_key) {
    auto voxels = deps.get<Voxels>(voxel_key);
    using SurfaceVoxels = decltype(voxels->surfaceVoxels());
    return std::make_shared<SurfaceVoxels>(voxels->surfaceVoxels());
  }
};

struct SurfaceVertices {
  auto operator()(ResourceDeps& deps, int voxel_key) {
    auto voxels = deps.get<Voxels>(voxel_key);
    using SurfaceVertices = decltype(voxels->surfaceVertices());
    return std::make_shared<SurfaceVertices>(voxels->surfaceVertices());
  }
};

// Provides batch level access to voxel arrays from within resource factories.
class VoxelAccessor {
 public:
  VoxelAccessor(ResourceDeps& deps)
      : deps_(deps),
        octree_(deps.get<WorldOctree>()),
        config_(deps.get<VoxelConfig>()) {}

  bool insideWorld(float x, float y, float z) {
    auto [x0, y0, z0, x1, y1, z1] = octree_->cellBox(0);
    return x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1;
  }

  uint32_t get(int x, int y, int z) {
    if (insideWorld(x, y, z)) {
      auto voxel_key = config_->voxelKey(x, y, z);
      auto [x0, y0, z0, x1, y1, z1] = config_->voxelBox(voxel_key);
      if (!voxel_cache_.count(voxel_key)) {
        voxel_cache_[voxel_key] = deps_.get<Voxels>(voxel_key);
      }
      return voxel_cache_.at(voxel_key)->get(x - x0, y - y0, z - z0);
    }
    return 0;
  }

  uint32_t get(float x, float y, float z) {
    return get(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
  }

 private:
  ResourceDeps& deps_;
  std::shared_ptr<Octree> octree_;
  std::shared_ptr<VoxelConfigData> config_;
  std::unordered_map<int, std::shared_ptr<VoxelArray>> voxel_cache_;
};

// Provides batch level mutation of voxel arrays.
class VoxelMutator {
 public:
  VoxelMutator(std::shared_ptr<Resources> resources)
      : resources_(resources),
        octree_(resources->get<WorldOctree>()),
        config_(resources->get<VoxelConfig>()) {}

  ~VoxelMutator() {
    auto world_db = resources_->get<WorldTable>();
    for (int voxel_key : mutated_) {
      auto va = voxel_cache_.at(voxel_key);
      world_db->setObject<VoxelArray>(format("voxels/%1%", voxel_key), *va);
      resources_->invalidate<Voxels>(voxel_key);
    }
  }

  bool insideWorld(float x, float y, float z) {
    auto [x0, y0, z0, x1, y1, z1] = octree_->cellBox(0);
    return x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1;
  }

  VoxelArray& cachedVoxelArray(int voxel_key) {
    if (!voxel_cache_.count(voxel_key)) {
      voxel_cache_[voxel_key] = resources_->get<Voxels>(voxel_key);
    }
    return *voxel_cache_.at(voxel_key);
  }

  uint32_t get(int x, int y, int z) {
    if (insideWorld(x, y, z)) {
      auto voxel_key = config_->voxelKey(x, y, z);
      auto [x0, y0, z0, x1, y1, z1] = config_->voxelBox(voxel_key);
      return cachedVoxelArray(voxel_key).get(x - x0, y - y0, z - z0);
    }
    return 0;
  }

  void set(int x, int y, int z, uint32_t value) {
    if (insideWorld(x, y, z)) {
      auto voxel_key = config_->voxelKey(x, y, z);
      auto [x0, y0, z0, x1, y1, z1] = config_->voxelBox(voxel_key);
      cachedVoxelArray(voxel_key).set(x - x0, y - y0, z - z0, value);
      mutated_.insert(voxel_key);
    }
  }

  uint32_t get(float x, float y, float z) {
    return get(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
  }

  void set(float x, float y, float z, uint32_t value) {
    return set(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(z),
        value);
  }

 private:
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<Octree> octree_;
  std::shared_ptr<VoxelConfigData> config_;
  std::unordered_map<int, std::shared_ptr<VoxelArray>> voxel_cache_;
  std::unordered_set<int> mutated_;
};

}  // namespace tequila