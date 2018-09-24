#pragma once

#include <cereal/archives/json.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <sstream>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/js.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

struct WorldName : public SeedResource<WorldName, std::string> {};

struct WorldCamera : public SeedResource<WorldCamera, std::shared_ptr<Camera>> {
};

struct WorldLight
    : public SeedResource<WorldLight, std::shared_ptr<glm::vec3>> {};

struct WorldJsContext
    : public SeedResource<WorldJsContext, std::shared_ptr<JsContext>> {};

struct WorldTable {
  auto operator()(const Resources& resources) {
    return std::make_shared<Table>(resources.get<WorldName>());
  }
};

struct WorldOctree {
  auto operator()(const Resources& resources) {
    auto json_config = resources.get<WorldTable>()->getJson("octree_config");
    return std::make_shared<Octree>(
        json_config.get<size_t>("leaf_size"),
        json_config.get<size_t>("grid_size"));
  }
};

struct VisibleCells {
  auto operator()(const Resources& resources) {
    auto octree = resources.get<WorldOctree>();
    auto camera = resources.get<WorldCamera>();
    auto cells = computeVisibleCells(*camera, *octree);
    return std::make_shared<std::vector<int64_t>>(std::move(cells));
  }
};

auto makeWorldCamera() {
  auto camera = std::make_shared<Camera>();
  camera->position = glm::vec3(0.0f, 0.0f, 0.0f);
  camera->view = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
  camera->fov = glm::radians(45.0f);
  camera->aspect = 4.0f / 3.0f;
  camera->near_distance = 0.1f;
  camera->far_distance = 100.0f;
  return camera;
}

auto makeWorldLight() {
  return std::make_shared<glm::vec3>(
      glm::normalize(glm::vec3(-2.0f, 4.0f, 1.0f)));
}

auto makeJsContext() {
  return std::make_shared<JsContext>();
}

auto makeWorldResources(const std::string& world_name) {
  return std::make_shared<Resources>(
      ResourcesBuilder()
          .withSeed<WorldCamera>(makeWorldCamera())
          .withSeed<WorldJsContext>(makeJsContext())
          .withSeed<WorldLight>(makeWorldLight())
          .withSeed<WorldName>(world_name)
          .build());
}

}  // namespace tequila