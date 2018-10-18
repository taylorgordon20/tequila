#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

#include "src/common/camera.hpp"
#include "src/common/meshes.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/shaders.hpp"
#include "src/common/textures.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

struct SkyData {
  Mesh mesh;
  std::shared_ptr<TextureCube> texture;
  glm::mat4 transform;
  SkyData(Mesh mesh, std::shared_ptr<TextureCube> texture, glm::mat4 transform)
      : mesh(std::move(mesh)),
        texture(std::move(texture)),
        transform(std::move(transform)) {}
};

struct SkyMap {
  auto operator()(const Resources& resources) {
    auto pixels = loadPngToTensor("images/sky_map_clouds.png");
    int h = pixels.dimension(0) / 3;
    int w = pixels.dimension(1) / 4;

    // NOTE: We need to invert every face due to OpenGL cube map inconsistency.
    std::vector<ImageTensor> faces(6);
    faces[0] = invertY(subImage(pixels, 2 * w, h, w, h));
    faces[1] = invertY(subImage(pixels, 0, h, w, h));
    faces[2] = invertY(subImage(pixels, w, 2 * h, w, h));
    faces[3] = invertY(subImage(pixels, w, 0, w, h));
    faces[4] = invertY(subImage(pixels, w, h, w, h));
    faces[5] = invertY(subImage(pixels, 3 * w, h, w, h));
    return std::make_shared<TextureCube>(std::move(faces));
  }
};

struct Sky {
  auto operator()(const Resources& resources) {
    static auto kPositions = [] {
      Eigen::Matrix3Xf ret(3, 6);
      ret.setZero();
      ret.row(0) << -1, 1, 1, 1, -1, -1;
      ret.row(1) << -1, -1, 1, 1, 1, -1;
      return ret;
    }();

    // Compute a rotation based on the current light location. We assume that
    // the sky map's light is directed positively along x in the xz-plane.
    const auto& light = *resources.get<WorldLight>();
    auto angle = std::atan2(-light[2], light[0]);

    return std::make_shared<SkyData>(
        MeshBuilder().setPositions(kPositions).build(),
        resources.get<SkyMap>(),
        glm::rotate(glm::mat4(1), angle, glm::vec3(0.0f, 1.0f, 0.0f)));
  }
};

struct SkyShader {
  auto operator()(const Resources& resources) {
    return std::make_shared<ShaderProgram>(std::vector<ShaderSource>{
        makeVertexShader(loadFile("shaders/sky.vert.glsl")),
        makeFragmentShader(loadFile("shaders/sky.frag.glsl")),
    });
  }
};

class SkyRenderer {
 public:
  SkyRenderer(std::shared_ptr<Resources> resources) : resources_(resources) {}

  void draw() const {
    auto camera = resources_->get<WorldCamera>();
    auto sky = resources_->get<Sky>();
    auto shader = resources_->get<SkyShader>();
    shader->run([&] {
      // Set uniforms.
      shader->uniform("view_matrix", camera->viewMatrix() * sky->transform);
      shader->uniform("projection_matrix", camera->projectionMatrix());

      // Bind the sky's cube map texture.
      TextureCubeBinding cube_map(*sky->texture, 0);
      shader->uniform("cube_map", cube_map.location());

      // Draw the sky box.
      gl::glDisable(gl::GL_DEPTH_TEST);
      sky->mesh.draw(*shader);
      gl::glEnable(gl::GL_DEPTH_TEST);
    });
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<SkyRenderer> gen(const Registry& registry) {
  return std::make_shared<SkyRenderer>(registry.get<Resources>());
}

}  // namespace tequila