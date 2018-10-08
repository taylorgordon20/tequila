#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/camera.hpp"
#include "src/common/data.hpp"
#include "src/common/maps.hpp"
#include "src/common/meshes.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/spatial.hpp"
#include "src/common/stats.hpp"
#include "src/common/textures.hpp"
#include "src/common/timers.hpp"
#include "src/common/voxels.hpp"
#include "src/worlds/core.hpp"
#include "src/worlds/styles.hpp"
#include "src/worlds/terrain.hpp"

namespace tequila {

enum TerrainSliceDir {
  LEFT = 0,
  RIGHT = 1,
  DOWN = 2,
  UP = 3,
  BACK = 4,
  FRONT = 5
};

inline auto terrainSliceOrigin(TerrainSliceDir dir) {
  static const std::vector<glm::vec3> kOrigins = {
      glm::vec3(0.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),
  };
  return kOrigins.at(dir);
}

inline auto terrainSliceNormal(TerrainSliceDir dir) {
  static const std::vector<glm::vec3> kNormals = {
      glm::vec3(-1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, -1.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, -1.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),
  };
  return kNormals.at(dir);
}

inline auto terrainSliceTangent(TerrainSliceDir dir) {
  static const std::vector<glm::vec3> kTangents = {
      glm::vec3(0.0f, 0.0f, 1.0f),
      glm::vec3(0.0f, 0.0f, -1.0f),
      glm::vec3(0.0f, 0.0f, -1.0f),
      glm::vec3(0.0f, 0.0f, 1.0f),
      glm::vec3(-1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
  };
  return kTangents.at(dir);
}

inline auto terrainSliceCotangent(TerrainSliceDir dir) {
  static const std::vector<glm::vec3> kCotangents = {
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(1.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f),
  };
  return kCotangents.at(dir);
}

struct TerrainVoxelKeys {
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

using TerrainSliceKey = std::tuple<int64_t, TerrainSliceDir>;
using TerrainSliceFace = std::tuple<int, int, int, int64_t>;

// Generates the keys of the terrain slices that should be rendered.
struct TerrainSliceKeys {
  auto operator()(const Resources& resources) {
    auto octree = resources.get<WorldOctree>();

    // Compute the depth of the voxel arrays.
    // TODO: Move this into an offline terrain config computation.
    auto voxel_level = 0;
    octree->search([&](int64_t cell) {
      auto voxel_keys = resources.get<TerrainVoxelKeys>(cell);
      ENFORCE(voxel_keys->size() >= 1);
      if (voxel_keys->size() == 1) {
        voxel_level = octree->cellLevel(cell);
        return false;
      }
      return true;
    });

    // Compute all visible cells at the level of the voxel arrays.
    std::unordered_set<int64_t> voxel_cells;
    auto cells = *resources.get<VisibleCells>();
    for (int i = 0; i < cells.size(); i += 1) {
      auto cell = cells.at(i);
      if (octree->cellLevel(cell) < voxel_level) {
        for (int j = 0; j < 8; j += 1) {
          cells.push_back(8 * cell + 1 + j);
        }
      } else if (octree->cellLevel(cell) > voxel_level) {
        cells.push_back(octree->cellParent(cell));
      } else {
        voxel_cells.insert(cell);
      }
    }

    // Output all relevant slices for the given cells.
    // TODO: Do back-face culling of slices here.
    // auto camera = resources.get<WorldCamera>();
    auto ret = std::make_shared<std::vector<TerrainSliceKey>>();
    for (auto cell : voxel_cells) {
      // auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);
      ret->emplace_back(cell, LEFT);
      ret->emplace_back(cell, RIGHT);
      ret->emplace_back(cell, DOWN);
      ret->emplace_back(cell, UP);
      ret->emplace_back(cell, BACK);
      ret->emplace_back(cell, FRONT);
    }
    return ret;
  }
};

struct TerrainSliceVoxels {
  auto operator()(const Resources& resources, int64_t cell) {
    auto voxel_keys = resources.get<TerrainVoxelKeys>(cell);
    ENFORCE(voxel_keys->size() == 1);
    return resources.get<TerrainVoxels>(voxel_keys->front());
  }
};

// Computes all faces.
struct TerrainSliceSurfaceVoxels {
  auto operator()(const Resources& resources, int64_t cell) {
    auto voxels = resources.get<TerrainSliceVoxels>(cell);
    using SurfaceVoxels = decltype(voxels->surfaceVoxels());
    return std::make_shared<SurfaceVoxels>(voxels->surfaceVoxels());
  }
};

// Computes all faces.
struct TerrainSliceFaces {
  auto operator()(const Resources& resources, TerrainSliceKey key) {
    auto cell = std::get<0>(key);
    auto sdir = std::get<1>(key);

    // Fetch the bounding box of this slice's terrain shard.
    auto octree = resources.get<WorldOctree>();
    auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);

    // Get the voxel array in which this cell is embedded.
    auto normal = terrainSliceNormal(sdir);
    auto voxels = resources.get<TerrainSliceVoxels>(cell);
    auto surface_voxels = resources.get<TerrainSliceSurfaceVoxels>(cell);
    auto faces = std::make_shared<std::vector<TerrainSliceFace>>();
    for (auto [x, y, z] : *surface_voxels) {
      int nx = x + static_cast<int>(normal[0]);
      int ny = y + static_cast<int>(normal[1]);
      int nz = z + static_cast<int>(normal[2]);
      int ox = nx < 0 || nx >= voxels->size();
      int oy = ny < 0 || ny >= voxels->size();
      int oz = nz < 0 || nz >= voxels->size();
      if (ox || oy || oz || !voxels->get(nx, ny, nz)) {
        faces->emplace_back(x0 + x, y0 + y, z0 + z, voxels->get(x, y, z));
      }
    }
    return faces;
  }
};

struct TerrainSliceData {
  Mesh mesh;
  glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec3 cotangent;
  std::shared_ptr<TextureArray> color_map;
  std::shared_ptr<TextureArray> normal_map;

  TerrainSliceData(
      Mesh mesh,
      glm::vec3 normal,
      glm::vec3 tangent,
      glm::vec3 cotangent,
      std::shared_ptr<TextureArray> color_map,
      std::shared_ptr<TextureArray> normal_map)
      : mesh(std::move(mesh)),
        normal(std::move(normal)),
        tangent(std::move(tangent)),
        cotangent(std::move(cotangent)),
        color_map(std::move(color_map)),
        normal_map(std::move(normal_map)) {}

  auto modelViewMatrix(const Camera& camera) {
    return camera.viewMatrix() * mesh.transform();
  }

  auto normalMatrix(const Camera& camera) {
    return glm::inverse(glm::transpose(glm::mat3(modelViewMatrix(camera))));
  }
};

// Creates the mesh of a terrain slice at a given size.
struct TerrainSlice {
  auto operator()(const Resources& resources, TerrainSliceKey key) {
    // Load the faces for this slice.
    auto faces = resources.get<TerrainSliceFaces>(key);
    if (faces->empty()) {
      return std::shared_ptr<TerrainSliceData>(nullptr);
    }

    // Look up the texture maps for this face since we need to specify vertex
    // attributes to point each face to its corresponding texture map indices.
    auto color_maps = resources.get<TerrainStylesColorMap>();
    auto normal_maps = resources.get<TerrainStylesNormalMap>();

    // Look up the surface vectors for the current direction.
    auto dir = std::get<1>(key);
    auto nor = terrainSliceNormal(dir);
    auto tan = terrainSliceTangent(dir);
    auto cot = terrainSliceCotangent(dir);
    auto pos = terrainSliceOrigin(dir);

    // Prepare primitives to Eigen-ify the mesh construction.
    auto ones_row = Eigen::Matrix<float, 1, 6>::Ones();
    Eigen::Matrix<float, 3, 6> dir_face;
    dir_face.col(0) << 0, 0, 0;
    dir_face.col(1) << tan[0], tan[1], tan[2];
    dir_face.col(2) << tan[0] + cot[0], tan[1] + cot[1], tan[2] + cot[2];
    dir_face.col(3) << tan[0] + cot[0], tan[1] + cot[1], tan[2] + cot[2];
    dir_face.col(4) << cot[0], cot[1], cot[2];
    dir_face.col(5) << 0, 0, 0;
    dir_face.row(0) += pos[0] * ones_row;
    dir_face.row(1) += pos[1] * ones_row;
    dir_face.row(2) += pos[2] * ones_row;

    // Construct the mesh's vertex attribute array.
    Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 6 * faces->size());
    Eigen::Matrix<float, 2, Eigen::Dynamic> indices(2, 6 * faces->size());
    for (int i = 0; i < faces->size(); i += 1) {
      auto [fx, fy, fz, style] = faces->at(i);

      // Positions.
      positions.row(0).segment(6 * i, 6) = fx * ones_row + dir_face.row(0);
      positions.row(1).segment(6 * i, 6) = fy * ones_row + dir_face.row(1);
      positions.row(2).segment(6 * i, 6) = fz * ones_row + dir_face.row(2);

      // Texture indices.
      // HACK: These indices are inappropriately shoved into texture coords.
      // TODO: Refactor mesh library into vertex buffer wrapper that supports
      // arbitrarily packed and typed vertex attributes.
      auto color_index = get_or(color_maps->index, style, 0);
      auto normal_index = get_or(normal_maps->index, style, 0);
      indices.row(0).segment(6 * i, 6) = color_index * ones_row;
      indices.row(1).segment(6 * i, 6) = normal_index * ones_row;
    }

    // Set the final slice data.
    return std::make_shared<TerrainSliceData>(
        MeshBuilder()
            .setPositions(std::move(positions))
            .setTexCoords(std::move(indices))
            .build(),
        std::move(nor),
        std::move(tan),
        std::move(cot),
        color_maps->texture_array,
        normal_maps->texture_array);
  }
};

struct TerrainShader {
  auto operator()(const Resources& resources) {
    return std::make_shared<ShaderProgram>(std::vector<ShaderSource>{
        makeVertexShader(loadFile("shaders/terrain.vert.glsl")),
        makeFragmentShader(loadFile("shaders/terrain.frag.glsl")),
    });
  }
};

class TerrainRenderer {
 public:
  TerrainRenderer(
      std::shared_ptr<Resources> resources, std::shared_ptr<Stats> stats)
      : resources_(resources), stats_(stats) {}

  void draw() const {
    StatsUpdate stats(stats_);

    // Fetch globals needed to render terrain.
    auto light = resources_->get<WorldLight>();
    auto camera = resources_->get<WorldCamera>();
    auto shader = resources_->get<TerrainShader>();

    shader->run([&] {
      shader->uniform("light", *light);
      shader->uniform("projection_matrix", camera->projectionMatrix());

      // Remder the terrain slices visible to the currewnt camera.
      auto slice_keys = resources_->get<TerrainSliceKeys>();
      for (const auto& key : *slice_keys) {
        auto slice = resources_->get<TerrainSlice>(key);
        if (!slice) {
          continue;
        }

        // Define geometric uniforms.
        shader->uniform("modelview_matrix", slice->modelViewMatrix(*camera));
        shader->uniform("normal_matrix", slice->normalMatrix(*camera));
        shader->uniform("normal", slice->normal);
        shader->uniform("tangent", slice->tangent);
        shader->uniform("cotangent", slice->cotangent);

        // Define texture uniforms.
        TextureArrayBinding color_map(*slice->color_map, 0);
        TextureArrayBinding normal_map(*slice->normal_map, 1);
        shader->uniform("color_map", color_map.location());
        shader->uniform("normal_map", normal_map.location());

        // Draw the mesh.
        slice->mesh.draw(*shader);

        // Update stats.
        stats["slices_count"] += 1;
      }
    });
  }

 private:
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<Stats> stats_;
};

template <>
std::shared_ptr<TerrainRenderer> gen(const Registry& registry) {
  return std::make_shared<TerrainRenderer>(
      registry.get<Resources>(), registry.get<Stats>());
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
      if (x0 <= x && x < x1 && y0 <= y && y < y1 && z0 <= z && z < z1) {
        if (octree->cellLevel(cell) + 1 == octree->treeDepth()) {
          auto voxel_keys = resources_->get<TerrainVoxelKeys>(cell);
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
      voxel_fn(ix, iy, iz);
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

}  // namespace tequila