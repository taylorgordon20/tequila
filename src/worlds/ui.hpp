#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

#include "src/common/opengl.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/strings.hpp"

namespace tequila {

struct UIShader {
  auto operator()(const Resources& resources) {
    return std::make_shared<ShaderProgram>(std::vector<ShaderSource>{
        makeVertexShader(loadFile("shaders/ui.vert.glsl")),
        makeFragmentShader(loadFile("shaders/ui.frag.glsl")),
    });
  }
};

struct UINode {
  std::string kind;
  std::unordered_map<std::string, std::string> attr;
};

struct UITree {
  std::unordered_map<std::string, UINode> nodes;
};

struct WorldUI : public SeedResource<WorldUI, std::shared_ptr<UITree>> {};

// The ready-to-render format of a simple colored rectangle UI node.
struct RectNode {
  Mesh mesh;
  RectNode(Mesh mesh) : mesh(std::move(mesh)) {}
};

// Builds the ready-to-render format of a rect node.
struct WorldRectNode {
  std::shared_ptr<RectNode> operator()(
      const Resources& resources, const std::string& id) {
    const auto& ui_node = resources.get<WorldUI>()->nodes.at(id);
    auto x = to<float>(ui_node.attr.at("x"));
    auto y = to<float>(ui_node.attr.at("y"));
    auto w = to<float>(ui_node.attr.at("width"));
    auto h = to<float>(ui_node.attr.at("height"));
    auto rgba = to<uint32_t>(ui_node.attr.at("color"));

    // Parse out the rect's geometry.
    Eigen::Matrix<float, 3, 6> positions;
    positions.row(0) << x, x, x + w, x + w, x, x + w;
    positions.row(1) << y, y + h, y, y, y + h, y + h;
    positions.row(2) << -1, -1, -1, -1, -1, -1;

    // Parse out the rect's color.
    Eigen::Matrix<float, 4, 6> colors;
    colors.setOnes();
    colors.row(0) *= ((rgba >> 24) & 0xFF) / 255.0f;
    colors.row(1) *= ((rgba >> 16) & 0xFF) / 255.0f;
    colors.row(2) *= ((rgba >> 8) & 0xFF) / 255.0f;
    colors.row(3) *= (rgba & 0xFF) / 255.0f;

    return std::make_shared<RectNode>(MeshBuilder()
                                          .setPositions(std::move(positions))
                                          .setColors(std::move(colors))
                                          .build());
  }
};

// A resource to index the IDs of all "rect" nodes.
struct WorldRectNodes {
  std::shared_ptr<std::vector<std::string>> operator()(
      const Resources& resources) {
    auto ui = resources.get<WorldUI>();
    auto ret = std::make_shared<std::vector<std::string>>();
    for (const auto& pair : ui->nodes) {
      if (pair.second.kind == "rect") {
        ret->push_back(pair.first);
      }
    }
    return ret;
  }
};

class RectUIRenderer {
 public:
  RectUIRenderer(std::shared_ptr<Resources> resources)
      : resources_(resources) {}
  void draw(ShaderProgram& shader) {
    auto rect_ids = resources_->get<WorldRectNodes>();
    for (const auto& rect_id : *rect_ids) {
      resources_->get<WorldRectNode>(rect_id)->mesh.draw(shader);
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<RectUIRenderer> gen(const Registry& registry) {
  return std::make_shared<RectUIRenderer>(registry.get<Resources>());
}

struct TextUINode {
  glm::vec3 position;
};

class TextUIRenderer {
 public:
  TextUIRenderer(std::shared_ptr<Resources> resources)
      : resources_(resources) {}
  void draw(ShaderProgram& shader) {}

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<TextUIRenderer> gen(const Registry& registry) {
  return std::make_shared<TextUIRenderer>(registry.get<Resources>());
}

class UIRenderer {
 public:
  UIRenderer(
      std::shared_ptr<Window> window,
      std::shared_ptr<Resources> resources,
      std::shared_ptr<RectUIRenderer> rect_renderer,
      std::shared_ptr<TextUIRenderer> text_renderer)
      : window_(window),
        resources_(resources),
        rect_renderer_(rect_renderer),
        text_renderer_(text_renderer) {}

  void draw() {
    int width, height;
    window_->call<glfwGetFramebufferSize>(&width, &height);
    auto ortho_mat = glm::ortho<float>(
        0.0,
        static_cast<float>(width),
        0.0,
        static_cast<float>(height),
        0.0,
        1000.0);

    auto shader = resources_->get<UIShader>();
    shader->run([&] {
      gl::glDisable(gl::GL_DEPTH_TEST);
      gl::glEnable(gl::GL_BLEND);
      gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
      shader->uniform("projection_matrix", ortho_mat);
      rect_renderer_->draw(*shader);
      text_renderer_->draw(*shader);
      gl::glDisable(gl::GL_BLEND);
      gl::glEnable(gl::GL_DEPTH_TEST);
    });
  }

 private:
  std::shared_ptr<Window> window_;
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<RectUIRenderer> rect_renderer_;
  std::shared_ptr<TextUIRenderer> text_renderer_;
};

template <>
std::shared_ptr<UIRenderer> gen(const Registry& registry) {
  return std::make_shared<UIRenderer>(
      registry.get<Window>(),
      registry.get<Resources>(),
      registry.get<RectUIRenderer>(),
      registry.get<TextUIRenderer>());
}

}  // namespace tequila