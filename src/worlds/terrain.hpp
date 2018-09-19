#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/spatial.hpp"
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
  TerrainRenderer(Resources& resources) : resources_(resources) {}

  void draw() const {
    const auto& cells = *resources_.get<VisibleCells>();

    // Figure out which voxel arrays need to be rendered.
    std::unordered_set<std::string> voxel_keys;
    for (auto cell : cells) {
      auto cell_keys = resources_.get<TerrainVoxelIndex>(cell);
      voxel_keys.insert(cell_keys->begin(), cell_keys->end());
    }

    // Draw each voxel array's mesh.
    auto shader = resources_.get<TerrainShader>();
    auto camera = resources_.get<WorldCamera>();
    auto light = resources_.get<WorldLight>();
    shader->run([&] {
      shader->uniform("light", *light);
      shader->uniform("projection_matrix", camera->projectionMatrix());
      for (const auto& voxel_key : voxel_keys) {
        auto mesh = resources_.get<TerrainMesh>(voxel_key);
        auto modelview = camera->viewMatrix() * mesh->transform();
        auto normal = glm::inverse(glm::transpose(glm::mat3(modelview)));
        shader->uniform("modelview_matrix", modelview);
        shader->uniform("normal_matrix", normal);
        mesh->draw(*shader);
      }
    });
  };

 private:
  Resources& resources_;
};

}  // namespace tequila