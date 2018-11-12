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

using TerrainSliceKey = std::tuple<int64_t, TerrainSliceDir>;
using TerrainSliceFace = std::tuple<int, int, int, int64_t>;

// Maps a terrain slice to its corresponding voxel array.
struct TerrainSliceVoxels {
  auto operator()(ResourceDeps& deps, int64_t cell) {
    auto voxel_keys = deps.get<VoxelKeys>(cell);
    ENFORCE(voxel_keys->size() == 1);
    return deps.get<Voxels>(voxel_keys->front());
  }
};

// Maps a terrain slice to its corresponding surface voxel array.
struct TerrainSliceSurfaceVoxels {
  auto operator()(ResourceDeps& deps, int64_t cell) {
    auto voxel_keys = deps.get<VoxelKeys>(cell);
    ENFORCE(voxel_keys->size() == 1);
    return deps.get<SurfaceVoxels>(voxel_keys->front());
  }
};

// Maps a terrain slice to its corresponding light map.
struct TerrainSliceVertexLights {
  auto operator()(ResourceDeps& deps, int64_t cell) {
    auto voxel_keys = deps.get<VoxelKeys>(cell);
    ENFORCE(voxel_keys->size() == 1);
    return deps.get<VertexLights>(voxel_keys->front());
  }
};

// Computes all faces.
struct TerrainSliceFaces {
  auto operator()(ResourceDeps& deps, TerrainSliceKey key) {
    StatsTimer timer(registryGet<Stats>(deps), "terrain_slice_faces");

    auto cell = std::get<0>(key);
    auto sdir = std::get<1>(key);

    // Fetch the bounding box of this slice's terrain shard.
    auto octree = deps.get<WorldOctree>();
    auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);

    // Get the voxel array in which this cell is embedded.
    auto normal = terrainSliceNormal(sdir);
    auto voxels = deps.get<TerrainSliceVoxels>(cell);
    auto surface_voxels = deps.get<TerrainSliceSurfaceVoxels>(cell);
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
  auto operator()(ResourceDeps& deps, TerrainSliceKey key) {
    // Load the faces for this slice.
    auto faces = deps.get<TerrainSliceFaces>(key);
    if (faces->empty()) {
      return std::shared_ptr<TerrainSliceData>();
    }

    StatsTimer timer(registryGet<Stats>(deps), "terrain_slice");

    // Look up the texture maps for this face since we need to specify
    // vertex attributes to point each face to its corresponding texture
    // map indices.
    const auto& styles = deps.get<TerrainStyles>()->styles;
    auto color_maps = deps.get<TerrainStylesColorMap>();
    auto normal_maps = deps.get<TerrainStylesNormalMap>();

    // Look up world coordinate information.
    auto cell = std::get<0>(key);
    auto octree = deps.get<WorldOctree>();
    auto [x0, y0, z0, x1, y1, z1] = octree->cellBox(cell);

    // Look up vertex lighting information for this slice's faces.
    auto vertex_lights = deps.get<TerrainSliceVertexLights>(cell);

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
      auto rgba = styles.at(style).colorVec();
      colors.row(0).segment(6 * i, 6) = rgba[0] * ones_row;
      colors.row(1).segment(6 * i, 6) = rgba[1] * ones_row;
      colors.row(2).segment(6 * i, 6) = rgba[2] * ones_row;

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
      auto color_index = get_or(color_maps->index, style, 0);
      auto normal_index = get_or(normal_maps->index, style, 0);
      indices.row(0).segment(6 * i, 6) = color_index * ones_row;
      indices.row(1).segment(6 * i, 6) = normal_index * ones_row;
    }

    // Set the final slice data.
    // NOTE: We need to execute this within the OpenGL context.
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
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
  auto operator()(ResourceDeps& deps, int64_t key) {
    StatsTimer timer(registryGet<Stats>(deps), "terrain_shard");

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
    StatsTimer timer(registryGet<Stats>(deps), "terrain_shard_keys");

    auto octree = deps.get<WorldOctree>();

    // Compute the depth of the voxel arrays.
    // TODO: Move this into an offline terrain config computation.
    auto voxel_level = 0;
    octree->search([&](int64_t cell) {
      auto voxel_keys = deps.get<VoxelKeys>(cell);
      ENFORCE(voxel_keys->size() >= 1);
      if (voxel_keys->size() == 1) {
        voxel_level = octree->cellLevel(cell);
        return false;
      }
      return true;
    });

    // Compute all visible cells at the level of the voxel arrays.
    std::unordered_set<int64_t> voxel_cells;
    auto cells = *deps.get<VisibleCells>();
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

    // Return the octree cell keys identifying each terrain shard.
    auto ret = std::make_shared<std::vector<uint64_t>>();
    ret->insert(ret->end(), voxel_cells.begin(), voxel_cells.end());
    return ret;
  }
};

struct TerrainShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
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
      std::shared_ptr<Resources> resources,
      std::shared_ptr<AsyncResources> async_resources,
      std::shared_ptr<Stats> stats)
      : resources_(resources),
        async_resources_(async_resources),
        stats_(stats) {}

  void draw() const {
    StatsUpdate stats(stats_);

    // Fetch globals needed to render terrain.
    auto light = resources_->get<WorldLight>();
    auto camera = resources_->get<WorldCamera>();
    auto shader = resources_->get<TerrainShader>();

    shader->run([&] {
      // Configure OpenGL pipeline state.
      gl::glEnable(gl::GL_DEPTH_TEST);
      Finally finally([&] { gl::glDisable(gl::GL_DEPTH_TEST); });

      // Set scene uniforms.
      shader->uniform("light", *light);
      shader->uniform("projection_matrix", camera->projectionMatrix());

      // Render the terrain slices visible to the current camera.
      auto shard_keys = resources_->get<TerrainShardKeys>();
      for (auto key : *shard_keys) {
        auto shard_opt = async_resources_->get_opt<TerrainShard>(key);
        if (!shard_opt) {
          continue;
        }

        for (auto slice : shard_opt.get()->slices) {
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
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<AsyncResources> async_resources_;
  std::shared_ptr<Stats> stats_;
};

template <>
inline std::shared_ptr<TerrainRenderer> gen(const Registry& registry) {
  return std::make_shared<TerrainRenderer>(
      registry.get<Resources>(),
      registry.get<AsyncResources>(),
      registry.get<Stats>());
}

}  // namespace tequila