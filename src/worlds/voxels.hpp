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
    glm::vec3 pos = glm::vec3(from[0] - 0.5f, from[1] - 0.5f, from[2] - 0.5f);
    glm::vec3 dir = glm::normalize(direction);
    int sx = std::signbit(dir[0]) ? -1 : 1;
    int sy = std::signbit(dir[1]) ? -1 : 1;
    int sz = std::signbit(dir[2]) ? -1 : 1;
    int ix = static_cast<int>(from[0]);
    int iy = static_cast<int>(from[1]);
    int iz = static_cast<int>(from[2]);
    float march_distance = 0;
    while (march_distance <= distance) {
      if (!voxel_fn(ix, iy, iz)) {
        break;
      }
      auto vx = glm::vec3(ix + sx, iy, iz);
      auto vy = glm::vec3(ix, iy + sy, iz);
      auto vz = glm::vec3(ix, iy, iz + sz);
      float proj_x = glm::dot(vx - pos, dir);
      float proj_y = glm::dot(vy - pos, dir);
      float proj_z = glm::dot(vz - pos, dir);
      float dx = glm::length(vx - proj_x * dir - pos);
      float dy = glm::length(vy - proj_y * dir - pos);
      float dz = glm::length(vz - proj_z * dir - pos);
      if (dx < dy && dx < dz) {
        ix += sx;
        march_distance = proj_x;
      } else if (dy < dx && dy < dz) {
        iy += sy;
        march_distance = proj_y;
      } else {
        iz += sz;
        march_distance = proj_z;
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