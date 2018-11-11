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

struct VoxelKeys {
  auto operator()(ResourceDeps& deps, int64_t cell) {
    auto world_db = deps.get<WorldTable>();
    auto json = world_db->getJson(format("cell_config/%1%/voxels", cell));
    return std::make_shared<std::vector<std::string>>(
        json.get<std::vector<std::string>>("voxel_keys"));
  }
};

struct Voxels {
  auto operator()(ResourceDeps& deps, const std::string& voxel_key) {
    auto world_db = deps.get<WorldTable>();
    return std::make_shared<VoxelArray>(
        world_db->getObject<VoxelArray>(voxel_key));
  }
};

struct SurfaceVoxels {
  auto operator()(ResourceDeps& deps, const std::string& voxel_key) {
    auto voxels = deps.get<Voxels>(voxel_key);
    using SurfaceVoxels = decltype(voxels->surfaceVoxels());
    return std::make_shared<SurfaceVoxels>(voxels->surfaceVoxels());
  }
};

struct SurfaceVertices {
  auto operator()(ResourceDeps& deps, const std::string& voxel_key) {
    auto voxels = deps.get<Voxels>(voxel_key);
    using SurfaceVertices = decltype(voxels->surfaceVertices());
    return std::make_shared<SurfaceVertices>(voxels->surfaceVertices());
  }
};

// Provides routines for accessing voxels.
// TODO: Modify this class to accept a "voxel_config" instead of the octree
// for determining the mapping of coordinate to voxel config.
class VoxelsSampler {
 public:
  template <typename Loader>
  VoxelsSampler(std::shared_ptr<Octree> octree, Loader&& loader)
      : octree_(octree), loader_(std::forward<Loader>(loader)) {}

  int32_t getVoxel(float x, float y, float z) {
    auto [x0, y0, z0, x1, y1, z1] = octree_->cellBox(0);
    if (x < x0 || x >= x1 || y < y0 || y >= y1 || z < z0 || z >= z1) {
      return 0;
    }

    updateCurrent(x, y, z);
    int ix = static_cast<int>(x - std::get<0>(current_box_));
    int iy = static_cast<int>(y - std::get<1>(current_box_));
    int iz = static_cast<int>(z - std::get<2>(current_box_));
    return current_vox_->get(ix, iy, iz);
  }

 private:
  void updateCurrent(float x, float y, float z) {
    if (current_vox_) {
      auto [x0, y0, z0, x1, y1, z1] = current_box_;
      if (x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1) {
        return;
      }
    }

    octree_->search([&](int64_t cell) {
      auto [x0, y0, z0, x1, y1, z1] = octree_->cellBox(cell);
      if (x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1) {
        // TODO: Use the voxel config here!!!
        if (x1 - x0 == 64) {
          current_box_ = std::tuple(x0, y0, z0, x1, y1, z1);
          current_vox_ = loader_(cell);
        } else {
          return true;
        }
      }
      return false;
    });

    ENFORCE(current_vox_);
  }

  std::shared_ptr<Octree> octree_;
  std::function<std::shared_ptr<VoxelArray>(int64_t)> loader_;
  std::tuple<int, int, int, int, int, int> current_box_;
  std::shared_ptr<VoxelArray> current_vox_;
};

// Provides routines for mutating voxels.
class VoxelsUtil {
 public:
  VoxelsUtil(std::shared_ptr<Resources> resources)
      : resources_(std::move(resources)) {}

  auto getWorldCoords(VoxelArray& va, float x, float y, float z) {
    return glm::vec3(va.transform() * glm::vec4(x, y, z, 1.0f));
  }

  auto getVoxelCoords(VoxelArray& va, float x, float y, float z) {
    auto inv_voxel_transform = glm::inverse(va.transform());
    auto vp = inv_voxel_transform * glm::vec4(x, y, z, 1.0);
    return std::make_tuple(
        static_cast<int>(vp[0]),
        static_cast<int>(vp[1]),
        static_cast<int>(vp[2]));
  }

  auto getVoxelKey(float x, float y, float z) {
    boost::optional<std::string> ret;
    auto octree = resources_->get<WorldOctree>();
    octree->search([&](int64_t cell) {
      auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);
      if (x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1) {
        if (octree->cellLevel(cell) + 1 == octree->treeDepth()) {
          auto voxel_keys = resources_->get<VoxelKeys>(cell);
          ENFORCE(voxel_keys->size() == 1);
          ret = voxel_keys->front();
        } else {
          return true;
        }
      }
      return false;
    });
    return ret;
  }

  template <typename Function>
  void marchVoxels(
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

  void reloadVoxels(const std::string& voxel_key, VoxelArray& voxel_array) {
    auto world_db = resources_->get<WorldTable>();
    world_db->setObject<VoxelArray>(voxel_key, voxel_array);
    resources_->invalidate<Voxels>(voxel_key);
  }

  uint32_t getVoxel(float x, float y, float z) {
    auto voxel_key = getVoxelKey(x, y, z);
    if (voxel_key) {
      auto va = resources_->get<Voxels>(*voxel_key);
      auto [ix, iy, iz] = getVoxelCoords(*va, x, y, z);
      return va->get(ix, iy, iz);
    }
    return 0;
  }

  void setVoxel(float x, float y, float z, uint32_t value) {
    auto voxel_key = getVoxelKey(x, y, z);
    if (voxel_key) {
      auto va = resources_->get<Voxels>(*voxel_key);
      auto [ix, iy, iz] = getVoxelCoords(*va, x, y, z);
      va->set(ix, iy, iz, value);
      reloadVoxels(*voxel_key, *va);
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
inline std::shared_ptr<VoxelsUtil> gen(const Registry& registry) {
  return std::make_shared<VoxelsUtil>(registry.get<Resources>());
}

}  // namespace tequila