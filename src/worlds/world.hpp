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

struct WorldCopyShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.copy.frag.glsl")),
      });
    });
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

struct WorldPassthroughShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.passthrough.frag.glsl")),
      });
    });
  }
};

struct WorldDepthBlurShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/world.vert.glsl")),
          makeFragmentShader(loadFile("shaders/world.depthblur.frag.glsl")),
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
    scene_map_ = std::make_shared<MultisampleTextureOutput>(
        width, height, samples, gl::GL_RGBA8);
    depth_map_ = std::make_shared<MultisampleTextureOutput>(
        width, height, samples, gl::GL_DEPTH_COMPONENT);
    scene_fbo_ = std::make_shared<MultisampleFramebuffer>(
        makeFramebuffer(scene_map_, depth_map_));

    // Create texture and framebuffers to store small copies of scene buffers.
    copy_width_ = static_cast<int>((512.0f * width) / height);
    copy_height_ = 512;
    copy_color_map_ = std::make_shared<TextureOutput>(
        copy_width_, copy_height_, gl::GL_RGBA8);
    copy_depth_map_ = std::make_shared<TextureOutput>(
        copy_width_, copy_height_, gl::GL_RGBA8);
    copy_fbo_ = std::make_shared<Framebuffer>(
        std::tuple(copy_width_, copy_height_),
        std::vector<std::shared_ptr<TextureOutput>>{copy_color_map_,
                                                    copy_depth_map_},
        nullptr);

    // Create a new texture and framebuffer to store bloom blurred scene.
    // TODO: Use bilinear sampling over a smaller texture.
    bloom_map1_ = std::make_shared<TextureOutput>(copy_width_, copy_height_);
    bloom_map2_ = std::make_shared<TextureOutput>(copy_width_, copy_height_);
    bloom_fbo1_ = std::make_shared<Framebuffer>(makeFramebuffer(bloom_map1_));
    bloom_fbo2_ = std::make_shared<Framebuffer>(makeFramebuffer(bloom_map2_));

    // Create a new texture and framebuffer to store depth blurred scene.
    // TODO: Use bilinear sampling over a smaller texture.
    boken_map1_ = std::make_shared<TextureOutput>(copy_width_, copy_height_);
    boken_map2_ = std::make_shared<TextureOutput>(copy_width_, copy_height_);
    boken_fbo1_ = std::make_shared<Framebuffer>(makeFramebuffer(boken_map1_));
    boken_fbo2_ = std::make_shared<Framebuffer>(makeFramebuffer(boken_map2_));
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

    // Stage 1: Copy scene and depth buffers to the copy maps.
    {
      FramebufferBinding fb(*copy_fbo_);
      gl::glViewport(0, 0, copy_width_, copy_height_);
      gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      auto shader = resources_->get<WorldCopyShader>();
      shader->run([&] {
        MultisampleTextureOutputBinding sb(*scene_map_, 0);
        MultisampleTextureOutputBinding db(*depth_map_, 1);
        shader->uniform("samples", kSamplesCount);
        shader->uniform("color_map", sb.location());
        shader->uniform("depth_map", db.location());
        frame_mesh->draw(*shader);
      });
    }

    // Stage 2: Apply light filter to the scene color map copy.
    {
      FramebufferBinding fb(*bloom_fbo1_);
      gl::glViewport(0, 0, copy_width_, copy_height_);
      gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      auto shader = resources_->get<WorldLightFilterShader>();
      shader->run([&] {
        TextureOutputBinding tb(*copy_color_map_, 0);
        shader->uniform("color_map", tb.location());
        frame_mesh->draw(*shader);
      });
    }

    // Stage 3: Blur light-filtered scene copy to produce bloom map.
    for (int blur_passes = 0; blur_passes < 10; blur_passes += 1) {
      // Blur the light map horizontally.
      {
        FramebufferBinding fb(*bloom_fbo2_);
        gl::glViewport(0, 0, copy_width_, copy_height_);
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
        gl::glViewport(0, 0, copy_width_, copy_height_);
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

    // Stage 4: Copy scene color map into boken map for depth blurring.
    {
      FramebufferBinding fb(*boken_fbo1_);
      gl::glViewport(0, 0, copy_width_, copy_height_);
      gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
      auto shader = resources_->get<WorldPassthroughShader>();
      shader->run([&] {
        TextureOutputBinding tb(*copy_color_map_, 0);
        shader->uniform("color_map", tb.location());
        frame_mesh->draw(*shader);
      });
    }

    // Stage 5: Depth-sensitive blur scene color map to produce boken map.
    for (int blur_passes = 0; blur_passes < 1; blur_passes += 1) {
      // Blur the boken map horizontally.
      {
        FramebufferBinding fb(*boken_fbo2_);
        gl::glViewport(0, 0, copy_width_, copy_height_);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        auto shader = resources_->get<WorldDepthBlurShader>();
        shader->run([&] {
          TextureOutputBinding kb(*boken_map1_, 0);
          TextureOutputBinding db(*copy_depth_map_, 1);
          shader->uniform("horizontal", 1);
          shader->uniform("color_map", kb.location());
          shader->uniform("depth_map", db.location());
          frame_mesh->draw(*shader);
        });
      }

      // Blur the dof map vertically.
      {
        FramebufferBinding fb(*boken_fbo1_);
        gl::glViewport(0, 0, copy_width_, copy_height_);
        gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        auto shader = resources_->get<WorldDepthBlurShader>();
        shader->run([&] {
          TextureOutputBinding kb(*boken_map2_, 0);
          TextureOutputBinding db(*copy_depth_map_, 1);
          shader->uniform("horizontal", 0);
          shader->uniform("color_map", kb.location());
          shader->uniform("depth_map", db.location());
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
        MultisampleTextureOutputBinding dmb(*depth_map_, 1);
        TextureOutputBinding bmb(*bloom_map1_, 2);
        TextureOutputBinding kmb(*boken_map1_, 3);
        shader->uniform("samples", kSamplesCount);
        shader->uniform("color_map", smb.location());
        shader->uniform("depth_map", dmb.location());
        shader->uniform("bloom_map", bmb.location());
        shader->uniform("boken_map", kmb.location());
        frame_mesh->draw(*shader);
      });
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<Window> window_;

  // Renderers for the world geometry.
  std::shared_ptr<SkyRenderer> sky_renderer_;
  std::shared_ptr<TerrainRenderer> terrain_renderer_;

  // World is rendered to the following multi-sampled buffers.
  std::shared_ptr<MultisampleTextureOutput> scene_map_;
  std::shared_ptr<MultisampleTextureOutput> depth_map_;
  std::shared_ptr<MultisampleFramebuffer> scene_fbo_;

  // Small copy of world scene for post-precessing filters.
  int copy_width_;
  int copy_height_;
  std::shared_ptr<TextureOutput> copy_color_map_;
  std::shared_ptr<TextureOutput> copy_depth_map_;
  std::shared_ptr<Framebuffer> copy_fbo_;

  // Post-processing buffers for generating bloom lighting.
  std::shared_ptr<TextureOutput> bloom_map1_;
  std::shared_ptr<TextureOutput> bloom_map2_;
  std::shared_ptr<Framebuffer> bloom_fbo1_;
  std::shared_ptr<Framebuffer> bloom_fbo2_;

  // Post-processing buffers for generating boken effect.
  std::shared_ptr<TextureOutput> boken_map1_;
  std::shared_ptr<TextureOutput> boken_map2_;
  std::shared_ptr<Framebuffer> boken_fbo1_;
  std::shared_ptr<Framebuffer> boken_fbo2_;
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