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

}  // namespace tequila