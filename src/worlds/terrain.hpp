#pragma once

#include <boost/optional.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/spatial.hpp"
#include "src/common/timers.hpp"
#include "src/common/voxels.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

struct TerrainShader {
  auto operator()(const Resources& resources) {
    return std::make_shared<ShaderProgram>(std::vector<ShaderSource>{
        makeVertexShader(loadFile("shaders/terrain.vert.glsl")),
        makeFragmentShader(loadFile("shaders/terrain.frag.glsl")),
    });
  }
};

struct TerrainVoxelIndex {
  auto operator()(const Resources& resources, int64_t cell) {
    auto world_db = resources.get<WorldTable>();
    auto json = world_db->getJson(format("cell_config/%1%/voxels", cell));
    return std::make_shared<std::vector<std::string>>(
        json.get<std::vector<std::string>>("voxel_keys"));
  }
};

struct TerrainVoxels {
  auto operator()(const Resources& resources, const std::string& voxel_key) {
    auto world_db = resources.get<WorldTable>();
    return std::make_shared<VoxelArray>(
        world_db->getObject<VoxelArray>(voxel_key));
  }
};

struct TerrainMesh {
  auto operator()(const Resources& resources, const std::string& voxel_key) {
    auto voxel_array = resources.get<TerrainVoxels>(voxel_key);
    return std::make_shared<Mesh>(voxel_array->toMesh());
  }
};

class TerrainRenderer {
 public:
  TerrainRenderer(std::shared_ptr<Resources> resources)
      : resources_(resources) {}

  void draw() const {
    auto cells = resources_->get<VisibleCells>();

    // Figure out which voxel arrays need to be rendered.
    std::unordered_set<std::string> voxel_keys;
    for (auto cell : *cells) {
      auto cell_keys = resources_->get<TerrainVoxelIndex>(cell);
      voxel_keys.insert(cell_keys->begin(), cell_keys->end());
    }

    // Draw each voxel array's mesh.
    auto shader = resources_->get<TerrainShader>();
    auto camera = resources_->get<WorldCamera>();
    auto light = resources_->get<WorldLight>();
    shader->run([&] {
      shader->uniform("light", *light);
      shader->uniform("projection_matrix", camera->projectionMatrix());
      for (const auto& voxel_key : voxel_keys) {
        auto mesh = resources_->get<TerrainMesh>(voxel_key);
        auto modelview = camera->viewMatrix() * mesh->transform();
        auto normal = glm::inverse(glm::transpose(glm::mat3(modelview)));
        shader->uniform("modelview_matrix", modelview);
        shader->uniform("normal_matrix", normal);
        mesh->draw(*shader);
      }
    });
  };

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<TerrainRenderer> gen(const Registry& registry) {
  return std::make_shared<TerrainRenderer>(registry.get<Resources>());
}

class TerrainUtil {
 public:
  TerrainUtil(std::shared_ptr<Resources> resources)
      : resources_(std::move(resources)) {}

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
      if (x0 <= x && x <= x1 && y0 <= y && y <= y1 && z0 <= z && z < z1) {
        if (octree->cellLevel(cell) + 1 == octree->treeDepth()) {
          auto voxel_keys = resources_->get<TerrainVoxelIndex>(cell);
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

  void reloadVoxels(const std::string& voxel_key, VoxelArray& voxel_array) {
    auto world_db = resources_->get<WorldTable>();
    world_db->setObject<VoxelArray>(voxel_key, voxel_array);
    resources_->invalidate<TerrainVoxels>(voxel_key);
  }

  uint32_t getVoxel(float x, float y, float z) {
    auto voxel_key = getVoxelKey(x, y, z);
    if (voxel_key) {
      auto va = resources_->get<TerrainVoxels>(*voxel_key);
      auto [ix, iy, iz] = getVoxelCoords(*va, x, y, z);
      return va->get(ix, iy, iz);
    }
    return 0;
  }

  void setVoxel(float x, float y, float z, uint32_t value) {
    auto voxel_key = getVoxelKey(x, y, z);
    if (voxel_key) {
      auto va = resources_->get<TerrainVoxels>(*voxel_key);
      auto [ix, iy, iz] = getVoxelCoords(*va, x, y, z);
      va->set(ix, iy, iz, value);
      reloadVoxels(*voxel_key, *va);
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<TerrainUtil> gen(const Registry& registry) {
  return std::make_shared<TerrainUtil>(registry.get<Resources>());
}

/*
class TerrainHandler {
 public:
  TerrainHandler(std::shared_ptr<Window> window, Resources& resources)
      : window_(std::move(window)), resources_(resources) {}

  void update(float dt) {
    if (window_->call<glfwGetKey>(GLFW_KEY_SPACE) == GLFW_PRESS) {
      addVoxel();
    } else if (window_->call<glfwGetKey>(GLFW_KEY_DELETE) == GLFW_PRESS) {
      delVoxel();
    }
  };

 private:
  struct VoxelHit {
    std::string voxel_key;
    std::shared_ptr<VoxelArray> voxel_array;
    std::tuple<int, int, int> voxel_pos;
  };

  auto getCameraVoxel() {
    auto octree = resources_.get<WorldOctree>();
    auto camera = resources_.get<WorldCamera>();
    auto position = camera->position + 2.0f * camera->view;

    // Find which voxel array needs to be updated.
    std::string voxel_key;
    octree->search([&](int64_t cell) {
      auto cell_box = octree->cellBox(cell);
      if (std::get<0>(cell_box) <= position[0] &&
          std::get<1>(cell_box) <= position[1] &&
          std::get<2>(cell_box) <= position[2] &&
          std::get<3>(cell_box) >= position[0] &&
          std::get<4>(cell_box) >= position[1] &&
          std::get<5>(cell_box) >= position[2]) {
        if (octree->cellLevel(cell) + 1 == octree->treeDepth()) {
          auto voxel_keys = resources_.get<TerrainVoxelIndex>(cell);
          ENFORCE(voxel_keys->size() == 1);
          voxel_key = voxel_keys->front();
        } else {
          return true;
        }
      }
      return false;
    });
    if (voxel_key.empty()) {
      return VoxelHit();
    }

    // Load the voxel array and figure out the hit coordinates.
    auto voxel_array = resources_.get<TerrainVoxels>(voxel_key);
    auto inv_voxel_transform = glm::inverse(voxel_array->transform());
    auto voxel_position = inv_voxel_transform * glm::vec4(position, 1.0f);
    return VoxelHit{
        voxel_key,
        voxel_array,
        {
            static_cast<int>(voxel_position[0]),
            static_cast<int>(voxel_position[1]),
            static_cast<int>(voxel_position[2]),
        },
    };
  }

  void addVoxel() {
    auto voxel_hit = getCameraVoxel();
    auto voxel_array = voxel_hit.voxel_array;
    auto vx = std::get<0>(voxel_hit.voxel_pos);
    auto vy = std::get<1>(voxel_hit.voxel_pos);
    auto vz = std::get<2>(voxel_hit.voxel_pos);
    if (!voxel_array || voxel_array->has(vx, vy, vz)) {
      return;
    }

    try {
      auto world_db = resources_.get<WorldTable>();
      voxel_array->set(vx, vy, vz, {255, 255, 255});
      world_db->setObject<VoxelArray>(voxel_hit.voxel_key, *voxel_array);
      resources_.invalidate<TerrainVoxels>(voxel_hit.voxel_key);
    } catch (const std::exception& e) {
      std::cout << "Failed to update voxel at coordinate: "
                << format("%1%,%2%,%3%", vx, vy, vz) << std::endl;
      std::cout << "Cause: " << e.what() << std::endl;
    }
  }

  void delVoxel() {
    auto voxel_hit = getCameraVoxel();
    auto voxel_array = voxel_hit.voxel_array;
    auto vx = std::get<0>(voxel_hit.voxel_pos);
    auto vy = std::get<1>(voxel_hit.voxel_pos);
    auto vz = std::get<2>(voxel_hit.voxel_pos);
    if (!voxel_array || !voxel_array->has(vx, vy, vz)) {
      return;
    }

    try {
      auto world_db = resources_.get<WorldTable>();
      voxel_array->del(vx, vy, vz);
      world_db->setObject<VoxelArray>(voxel_hit.voxel_key, *voxel_array);
      resources_.invalidate<TerrainVoxels>(voxel_hit.voxel_key);
    } catch (const std::exception& e) {
      std::cout << "Failed to update voxel at coordinate: "
                << format("%1%,%2%,%3%", vx, vy, vz) << std::endl;
      std::cout << "Cause: " << e.what() << std::endl;
    }
  }

  std::shared_ptr<Window> window_;
  Resources& resources_;
};
*/

}  // namespace tequila