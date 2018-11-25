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
#include "src/worlds/lights.hpp"
#include "src/worlds/opengl.hpp"
#include "src/worlds/styles.hpp"
#include "src/worlds/terrain.hpp"
#include "src/worlds/voxels.hpp"

namespace tequila {

enum TerrainSliceDir {
  LEFT = 0,
  RIGHT = 1,
  DOWN = 2,
  UP = 3,
  BACK = 4,
  FRONT = 5
};

inline std::ostream& operator<<(std::ostream& os, TerrainSliceDir& dir) {
  return os << static_cast<int>(dir);
}

inline auto terrainSliceStyleKey(int64_t style, TerrainSliceDir dir) {
  switch (dir) {
    case LEFT:
      return StyleIndexKey(style, "left");
    case RIGHT:
      return StyleIndexKey(style, "right");
    case DOWN:
      return StyleIndexKey(style, "bottom");
    case UP:
      return StyleIndexKey(style, "top");
    case BACK:
      return StyleIndexKey(style, "back");
    case FRONT:
      return StyleIndexKey(style, "front");
    default:
      throwError("Invalid terrain slice dir: %1%", dir);
  }
}

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

inline auto terrainSliceVertexOffsets(TerrainSliceDir dir) {
  static const std::vector<std::vector<std::tuple<int, int, int>>> kOffsets = {
      {
          std::tuple(0, 0, 0),
          std::tuple(0, 0, 1),
          std::tuple(0, 1, 0),
          std::tuple(0, 1, 1),
      },
      {
          std::tuple(1, 0, 1),
          std::tuple(1, 0, 0),
          std::tuple(1, 1, 1),
          std::tuple(1, 1, 0),
      },
      {
          std::tuple(0, 0, 1),
          std::tuple(0, 0, 0),
          std::tuple(1, 0, 1),
          std::tuple(1, 0, 0),
      },
      {
          std::tuple(0, 1, 0),
          std::tuple(0, 1, 1),
          std::tuple(1, 1, 0),
          std::tuple(1, 1, 1),
      },
      {
          std::tuple(1, 0, 0),
          std::tuple(0, 0, 0),
          std::tuple(1, 1, 0),
          std::tuple(0, 1, 0),
      },
      {
          std::tuple(0, 0, 1),
          std::tuple(1, 0, 1),
          std::tuple(0, 1, 1),
          std::tuple(1, 1, 1),
      },
  };
  return kOffsets.at(dir);
}

using TerrainSliceKey = std::tuple<int, TerrainSliceDir>;
using TerrainSliceFace = std::tuple<int, int, int, int64_t>;

// Computes all faces.
struct TerrainSliceFaces {
  auto operator()(ResourceDeps& deps, TerrainSliceKey key) {
    StatsTimer timer(deps.get<WorldStats>(), "terrain_slice_faces");

    auto shard_key = std::get<0>(key);
    auto shard_dir = std::get<1>(key);

    // Fetch the bounding box of this slice's terrain shard.
    auto voxel_config = deps.get<VoxelConfig>();
    auto [x0, y0, z0, x1, y1, z1] = voxel_config->voxelBox(shard_key);

    // Identify surface faces for the current terrain shard.
    VoxelAccessor accessor(deps);
    auto normal = terrainSliceNormal(shard_dir);
    auto surface_voxels = deps.get<SurfaceVoxels>(shard_key);
    auto faces = std::make_shared<std::vector<TerrainSliceFace>>();
    for (auto [ix, iy, iz] : *surface_voxels) {
      int x = x0 + ix;
      int y = y0 + iy;
      int z = z0 + iz;
      int nx = x + static_cast<int>(normal[0]);
      int ny = y + static_cast<int>(normal[1]);
      int nz = z + static_cast<int>(normal[2]);
      if (!accessor.get(nx, ny, nz)) {
        faces->emplace_back(x, y, z, accessor.get(x, y, z));
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
  auto operator()(ResourceDeps& deps, TerrainSliceKey key) {
    // Load the faces for this slice.
    auto faces = deps.get<TerrainSliceFaces>(key);
    if (faces->empty()) {
      return std::shared_ptr<TerrainSliceData>();
    }

    StatsTimer timer(deps.get<WorldStats>(), "terrain_slice");

    // Look up the texture maps for this face since we need to specify
    // vertex attributes to point each face to its corresponding texture
    // map indices.
    auto terrain_styles = deps.get<TerrainStyles>();
    auto color_maps = deps.get<TerrainStylesColorMap>();
    auto normal_maps = deps.get<TerrainStylesNormalMap>();

    // Fetch the bounding box of this slice's terrain shard.
    auto shard_key = std::get<0>(key);
    auto voxel_config = deps.get<VoxelConfig>();
    auto [x0, y0, z0, x1, y1, z1] = voxel_config->voxelBox(shard_key);

    // Look up vertex lighting information for this slice's faces.
    auto vertex_lights = deps.get<VertexLights>(shard_key);

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

    // Prepare vertex index deltas the each face
    auto vertex_offsets = terrainSliceVertexOffsets(dir);
    auto [x_00, y_00, z_00] = vertex_offsets.at(0);
    auto [x_01, y_01, z_01] = vertex_offsets.at(1);
    auto [x_10, y_10, z_10] = vertex_offsets.at(2);
    auto [x_11, y_11, z_11] = vertex_offsets.at(3);

    // Construct the mesh's vertex attribute array.
    Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 6 * faces->size());
    Eigen::Matrix<float, 3, Eigen::Dynamic> colors(3, 6 * faces->size());
    Eigen::Matrix<float, 2, Eigen::Dynamic> indices(2, 6 * faces->size());
    Eigen::Matrix<float, 3, Eigen::Dynamic> lights(3, 6 * faces->size());
    for (int i = 0; i < faces->size(); i += 1) {
      auto [fx, fy, fz, style] = faces->at(i);
      auto vx = fx - x0, vy = fy - y0, vz = fz - z0;

      // Positions.
      positions.row(0).segment(6 * i, 6) = fx * ones_row + dir_face.row(0);
      positions.row(1).segment(6 * i, 6) = fy * ones_row + dir_face.row(1);
      positions.row(2).segment(6 * i, 6) = fz * ones_row + dir_face.row(2);

      // Colors.
      if (auto style_ptr = get_ptr(terrain_styles->styles, style)) {
        auto rgba = style_ptr->colorVec();
        colors.row(0).segment(6 * i, 6) = rgba[0] * ones_row;
        colors.row(1).segment(6 * i, 6) = rgba[1] * ones_row;
        colors.row(2).segment(6 * i, 6) = rgba[2] * ones_row;
      } else {
        colors.block<3, 6>(0, 6 * i).setOnes();
      }

      // Lights.
      // HACK: These lights are inappropriately shoved into normal coords.
      // TODO: Refactor mesh library into vertex buffer wrapper that
      // supports arbitrarily packed and typed vertex attributes.
      auto light_00 = vertex_lights->at(vx + x_00, vy + y_00, vz + z_00);
      auto light_01 = vertex_lights->at(vx + x_01, vy + y_01, vz + z_01);
      auto light_10 = vertex_lights->at(vx + x_10, vy + y_10, vz + z_10);
      auto light_11 = vertex_lights->at(vx + x_11, vy + y_11, vz + z_11);
      lights(0, 6 * i) = light_00.global_occlusion;
      lights(0, 6 * i + 1) = light_01.global_occlusion;
      lights(0, 6 * i + 2) = light_11.global_occlusion;
      lights(0, 6 * i + 3) = light_11.global_occlusion;
      lights(0, 6 * i + 4) = light_10.global_occlusion;
      lights(0, 6 * i + 5) = light_00.global_occlusion;

      // Texture map layer indices.
      // HACK: These indices are inappropriately shoved into texture coords.
      // TODO: Refactor mesh library into vertex buffer wrapper that supports
      // arbitrarily packed and typed vertex attributes.
      auto style_index_key = terrainSliceStyleKey(style, dir);
      auto color_index = color_maps->indexOrDefault(style_index_key);
      auto normal_index = normal_maps->indexOrDefault(style_index_key);
      indices.row(0).segment(6 * i, 6) = color_index * ones_row;
      indices.row(1).segment(6 * i, 6) = normal_index * ones_row;
    }

    // Set the final slice data.
    // NOTE: We need to execute this within the OpenGL context.
    return deps.get<OpenGLExecutor>()->manage([&] {
      return new TerrainSliceData(
          MeshBuilder()
              .setPositions(std::move(positions))
              .setColors(std::move(colors))
              .setTexCoords(std::move(indices))
              .setNormals(std::move(lights))
              .build(),
          std::move(nor),
          std::move(tan),
          std::move(cot),
          color_maps->texture_array,
          normal_maps->texture_array);
    });
  }
};

struct TerrainShardData {
  std::vector<std::shared_ptr<TerrainSliceData>> slices;
};

// A collection of terrain slices to render. It's important to aggregate slices
// into a shard so that we can cull slices facing away from the camera and to
// ensure that all slices within a shard display updates atomically.
struct TerrainShard {
  auto operator()(ResourceDeps& deps, int key) {
    StatsTimer timer(deps.get<WorldStats>(), "terrain_shard");

    // Output all relevant slices for the given shard.
    // TODO: Do back-face culling of slices here.
    auto ret = std::make_shared<TerrainShardData>();
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, LEFT))) {
      ret->slices.push_back(slice);
    }
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, RIGHT))) {
      ret->slices.push_back(slice);
    }
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, DOWN))) {
      ret->slices.push_back(slice);
    }
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, UP))) {
      ret->slices.push_back(slice);
    }
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, BACK))) {
      ret->slices.push_back(slice);
    }
    if (auto slice = deps.get<TerrainSlice>(TerrainSliceKey(key, FRONT))) {
      ret->slices.push_back(slice);
    }
    return ret;
  }
};

// Returns the keys for the terrain shards that should be rendered.
struct TerrainShardKeys {
  auto operator()(ResourceDeps& deps) {
    StatsTimer timer(deps.get<WorldStats>(), "terrain_shard_keys");

    // Terrains shards are one-to-one with voxel array indices.
    auto octree = deps.get<WorldOctree>();
    auto voxel_config = deps.get<VoxelConfig>();
    auto shard_size = voxel_config->voxel_size;
    auto grid_size = voxel_config->grid_size;

    // Find all shard keys that intersect with the visible octree cells.
    std::unordered_set<int> shard_keys;
    for (auto cell : *deps.get<VisibleCells>()) {
      octree->search(
          [&](int64_t cell) {
            auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);
            if (x1 - x0 <= shard_size) {
              int sx = x0 / shard_size;
              int sy = y0 / shard_size;
              int sz = z0 / shard_size;
              shard_keys.insert(
                  sx + sy * grid_size + sz * grid_size * grid_size);
              return false;
            } else {
              return true;
            }
          },
          cell);
    }

    // Return the distinct keys as a vector.
    auto ret = std::make_shared<std::vector<int>>();
    ret->insert(ret->end(), shard_keys.begin(), shard_keys.end());
    return ret;
  }
};

struct TerrainShader {
  auto operator()(ResourceDeps& deps) {
    return deps.get<OpenGLExecutor>()->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/terrain.vert.glsl")),
          makeFragmentShader(loadFile("shaders/terrain.frag.glsl")),
      });
    });
  }
};

class TerrainRenderer {
 public:
  TerrainRenderer(
      std::shared_ptr<AsyncResources> async_resources,
      std::shared_ptr<Stats> stats)
      : resources_(async_resources), stats_(stats) {}

  void draw() const {
    StatsUpdate stats(stats_);
    StatsTimer loop_timer(stats_, "terrain_renderer");

    // Fetch globals needed to render terrain.
    auto light = resources_->syncGet<WorldLight>();
    auto camera = resources_->syncGet<WorldCamera>();
    auto shader = resources_->syncGet<TerrainShader>();

    shader->run([&] {
      using namespace std::chrono_literals;

      // Configure OpenGL pipeline state.
      gl::glEnable(gl::GL_DEPTH_TEST);
      Finally finally([&] { gl::glDisable(gl::GL_DEPTH_TEST); });

      // Set scene uniforms.
      shader->uniform("light", *light);
      shader->uniform("projection_matrix", camera->projectionMatrix());

      // Render the terrain slices visible to the current camera.
      auto shard_keys = resources_->syncGet<TerrainShardKeys>();
      for (auto key : *shard_keys) {
        auto opt_shard = resources_->optGet<TerrainShard>(key);
        if (!opt_shard) {
          continue;
        }
        for (auto slice : opt_shard.get()->slices) {
          // Set geometry uniforms.
          shader->uniform("modelview_matrix", slice->modelViewMatrix(*camera));
          shader->uniform("normal_matrix", slice->normalMatrix(*camera));
          shader->uniform("slice_normal", slice->normal);
          shader->uniform("slice_tangent", slice->tangent);
          shader->uniform("slice_cotangent", slice->cotangent);

          // Set texture uniforms.
          TextureArrayBinding color_map(*slice->color_map, 0);
          TextureArrayBinding normal_map(*slice->normal_map, 1);
          shader->uniform("color_map", color_map.location());
          shader->uniform("normal_map", normal_map.location());

          // Draw the mesh.
          slice->mesh.draw(*shader);

          // Update stats.
          stats["terrain_slices_count"] += 1;
        }
        stats["terrain_shards_count"] += 1;
      }
    });
  }

 private:
  std::shared_ptr<AsyncResources> resources_;
  std::shared_ptr<Stats> stats_;
};

template <>
inline std::shared_ptr<TerrainRenderer> gen(const Registry& registry) {
  return std::make_shared<TerrainRenderer>(
      registry.get<AsyncResources>(), registry.get<Stats>());
}

}  // namespace tequila