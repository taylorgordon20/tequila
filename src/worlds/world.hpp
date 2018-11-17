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

struct WorldLightFilterShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.lightfilter.frag.glsl")),
      });
    });
  }
};

struct WorldBlurShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.blur.frag.glsl")),
      });
    });
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

  void updateBuffers(int width, int height, int samples) {
    // Return if no update is required.
    if (scene_map_) {
      if (width == 0 || height == 0) {
        return;
      }
      if (std::tuple(width, height) == scene_map_->dimensions() &&
          samples == scene_map_->samples()) {
        return;
      }
    }

    // Create a new scene texture and framebuffer.
    // NOTE: We need to delete the framebuffers before the textures are deleted.
    scene_map_ =
        std::make_shared<MultisampleTextureOutput>(width, height, samples);
    scene_fbo_ =
        std::make_shared<MultisampleFramebuffer>(makeFramebuffer(scene_map_));

    // Create a new light map texture and framebuffer.
    // TODO: Use bilinear sampling over a smaller texture.
    bloom_width_ = static_cast<int>((512.0f * width) / height);
    bloom_height_ = 512;
    bloom_map1_ = std::make_shared<TextureOutput>(bloom_width_, bloom_height_);
    bloom_map2_ = std::make_shared<TextureOutput>(bloom_width_, bloom_height_);
    bloom_fbo1_ = std::make_shared<Framebuffer>(makeFramebuffer(bloom_map1_));
    bloom_fbo2_ = std::make_shared<Framebuffer>(makeFramebuffer(bloom_map2_));
  }

  auto getWindowSize() {
    int w, h;
    window_->call<glfwGetFramebufferSize>(&w, &h);
    return std::tuple(w, h);
  }

  void draw() {
    constexpr int kSamplesCount = 4;
    auto [window_width, window_height] = getWindowSize();
    updateBuffers(window_width, window_height, kSamplesCount);

    // Draw the sky and terrain to the scene framebuffer.
    // TODO: Add gamma correction for the terrain.
    {
      FramebufferBinding fb(*scene_fbo_);
      gl::glViewport(0, 0, window_width, window_height);
      gl::glClearColor(0.62f, 0.66f, 0.8f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      sky_renderer_->draw();
      terrain_renderer_->draw();
    }

    // All subsequent post-process rendering is simply to the frame mesh.
    auto frame_mesh = resources_->get<WorldFrameMesh>();

    // Compute the raw light map from the scene.
    {
      FramebufferBinding fb(*bloom_fbo1_);
      gl::glViewport(0, 0, bloom_width_, bloom_height_);
      gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      auto shader = resources_->get<WorldLightFilterShader>();
      shader->run([&] {
        MultisampleTextureOutputBinding tb(*scene_map_, 0);
        shader->uniform("samples", kSamplesCount);
        shader->uniform("color_map", tb.location());
        frame_mesh->draw(*shader);
      });
    }

    for (int blur_passes = 0; blur_passes < 10; blur_passes += 1) {
      // Blur the light map horizontally.
      {
        FramebufferBinding fb(*bloom_fbo2_);
        gl::glViewport(0, 0, bloom_width_, bloom_height_);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        auto shader = resources_->get<WorldBlurShader>();
        shader->run([&] {
          TextureOutputBinding tb(*bloom_map1_, 0);
          shader->uniform("horizontal", 1);
          shader->uniform("color_map", tb.location());
          frame_mesh->draw(*shader);
        });
      }

      // Blur the light map vertically.
      {
        FramebufferBinding fb(*bloom_fbo1_);
        gl::glViewport(0, 0, bloom_width_, bloom_height_);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        auto shader = resources_->get<WorldBlurShader>();
        shader->run([&] {
          TextureOutputBinding tb(*bloom_map2_, 0);
          shader->uniform("horizontal", 0);
          shader->uniform("color_map", tb.location());
          frame_mesh->draw(*shader);
        });
      }
    }

    // Render the scene map to the screen.
    {
      gl::glViewport(0, 0, window_width, window_height);
      gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      auto shader = resources_->get<WorldShader>();
      shader->run([&] {
        MultisampleTextureOutputBinding smb(*scene_map_, 0);
        TextureOutputBinding bmb(*bloom_map1_, 1);
        shader->uniform("samples", kSamplesCount);
        shader->uniform("color_map", smb.location());
        shader->uniform("bloom_map", bmb.location());
        frame_mesh->draw(*shader);
      });
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<SkyRenderer> sky_renderer_;
  std::shared_ptr<TerrainRenderer> terrain_renderer_;

  std::shared_ptr<MultisampleTextureOutput> scene_map_;
  std::shared_ptr<MultisampleFramebuffer> scene_fbo_;

  int bloom_width_;
  int bloom_height_;
  std::shared_ptr<TextureOutput> bloom_map1_;
  std::shared_ptr<TextureOutput> bloom_map2_;
  std::shared_ptr<Framebuffer> bloom_fbo1_;
  std::shared_ptr<Framebuffer> bloom_fbo2_;
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