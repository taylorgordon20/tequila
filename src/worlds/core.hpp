#pragma once

#include <cereal/archives/json.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <sstream>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/concurrency.hpp"
#include "src/common/data.hpp"
#include "src/common/errors.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/spatial.hpp"
#include "src/common/stats.hpp"
#include "src/worlds/opengl.hpp"

namespace tequila {

struct OpenGLExecutor : public SeedResource<
                            OpenGLExecutor,
                            std::shared_ptr<OpenGLContextExecutor>> {};

struct WorldStats : public SeedResource<WorldStats, std::shared_ptr<Stats>> {};

struct WorldName : public SeedResource<WorldName, std::string> {};

struct WorldCamera : public SeedResource<WorldCamera, std::shared_ptr<Camera>> {
};

struct WorldLight
    : public SeedResource<WorldLight, std::shared_ptr<glm::vec3>> {};

struct WorldTable {
  auto operator()(ResourceDeps& deps) {
    return std::make_shared<Table>(deps.get<WorldName>());
  }
};

struct WorldOctree {
  auto operator()(ResourceDeps& deps) {
    auto json_config = deps.get<WorldTable>()->getJson("octree_config");
    return std::make_shared<Octree>(
        json_config.get<size_t>("leaf_size"),
        json_config.get<size_t>("grid_size"));
  }
};

struct VisibleCells {
  auto operator()(ResourceDeps& deps) {
    auto octree = deps.get<WorldOctree>();
    auto camera = deps.get<WorldCamera>();
    auto cells = computeVisibleCells(*camera, *octree);
    return std::make_shared<std::vector<int64_t>>(std::move(cells));
  }
};

}  // namespace tequila
