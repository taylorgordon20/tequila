#pragma once

#include <cereal/archives/json.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <sstream>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/resources.hpp"
#include "src/common/spatial.hpp"
#include "src/common/window.hpp"

namespace tequila {

struct WorldWindow
    : public SingletonResource<WorldWindow, std::shared_ptr<Window>> {};

struct WorldName : public SingletonResource<WorldName, std::string> {};

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

struct WorldCamera
    : public SingletonResource<WorldCamera, std::shared_ptr<Camera>> {};

struct WorldLight
    : public SingletonResource<WorldLight, std::shared_ptr<glm::vec3>> {};

struct VisibleCells {
  auto operator()(const Resources& resources) {
    auto octree = resources.get<WorldOctree>();
    auto camera = resources.get<WorldCamera>();
    auto cells = computeVisibleCells(*camera, *octree);
    return std::make_shared<std::vector<int64_t>>(std::move(cells));
  }
};

}  // namespace tequila