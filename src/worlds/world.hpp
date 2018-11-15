#pragma once

#include <Eigen/Dense>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "src/common/framebuffers.hpp"
#include "src/common/resources.hpp"
#include "src/common/textures.hpp"
#include "src/common/window.hpp"
#include "src/worlds/sky.hpp"
#include "src/worlds/terrain.hpp"

namespace tequila {

struct WorldFrameMesh {
  auto operator()(ResourceDeps& deps) {
    static auto kPositions = [] {
      Eigen::Matrix3Xf ret(3, 6);
      ret.setZero();
      ret.row(0) << -1, 1, 1, 1, -1, -1;
      ret.row(1) << -1, -1, 1, 1, 1, -1;
      return ret;
    }();
    static auto kTexCoords = [] {
      Eigen::Matrix2Xf ret(2, 6);
      ret.row(0) << 0, 1, 1, 1, 0, 0;
      ret.row(1) << 0, 0, 1, 1, 1, 0;
      return ret;
    }();
    return std::make_shared<Mesh>(MeshBuilder()
                                      .setPositions(kPositions)
                                      .setTexCoords(kTexCoords)
                                      .build());
  }
};

struct WorldShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.frag.glsl")),
      });
    });
  }
};

class WorldRenderer {
 public:
  WorldRenderer(
      std::shared_ptr<Resources> resources,
      std::shared_ptr<Window> window,
      std::shared_ptr<SkyRenderer> sky_renderer,
      std::shared_ptr<TerrainRenderer> terrain_renderer)
      : resources_(resources),
        window_(window),
        sky_renderer_(sky_renderer),
        terrain_renderer_(terrain_renderer) {}

  void updateFramebuffer(int width, int height, int samples) {
    if (color_map_) {
      if (std::tuple(width, height) == color_map_->dimensions() &&
          samples == color_map_->samples()) {
        return;
      }
    }

    // Update required. Create a new texture and framebuffer.
    color_map_ = std::make_shared<TextureOutput>(width, height, samples);
    framebuffer_ = std::make_shared<Framebuffer>(*color_map_);
  }

  auto getWindowSize() {
    int w, h;
    window_->call<glfwGetFramebufferSize>(&w, &h);
    return std::tuple(w, h);
  }

  void draw() {
    constexpr int kSamplesCount = 4;
    auto [w, h] = getWindowSize();
    updateFramebuffer(w, h, kSamplesCount);

    // Draw the sky and terrain to the framebuffer.
    {
      FramebufferBinding fb(*framebuffer_);
      gl::glViewport(0, 0, w, h);
      gl::glClearColor(0.62f, 0.66f, 0.8f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      gl::glEnable(gl::GL_MULTISAMPLE);
      sky_renderer_->draw();
      terrain_renderer_->draw();
      gl::glDisable(gl::GL_MULTISAMPLE);
    }

    // Render the framebuffer to the screen.
    auto shader = resources_->get<WorldShader>();
    auto frame_mesh = resources_->get<WorldFrameMesh>();
    shader->run([&] {
      TextureOutputBinding tb(*color_map_, 0);
      shader->uniform("samples", kSamplesCount);
      shader->uniform("color_map", tb.location());
      frame_mesh->draw(*shader);
    });
  }

 private:
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<SkyRenderer> sky_renderer_;
  std::shared_ptr<TerrainRenderer> terrain_renderer_;
  std::shared_ptr<TextureOutput> color_map_;
  std::shared_ptr<Framebuffer> framebuffer_;
};

template <>
inline std::shared_ptr<WorldRenderer> gen(const Registry& registry) {
  return std::make_shared<WorldRenderer>(
      registry.get<Resources>(),
      registry.get<Window>(),
      registry.get<SkyRenderer>(),
      registry.get<TerrainRenderer>());
}

}  // namespace tequila