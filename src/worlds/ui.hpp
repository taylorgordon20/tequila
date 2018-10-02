#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>

#include "src/common/maps.hpp"
#include "src/common/opengl.hpp"
#include "src/common/registry.hpp"
#include "src/common/resources.hpp"
#include "src/common/strings.hpp"
#include "src/common/text.hpp"

namespace tequila {

struct UIShader {
  auto operator()(const Resources& resources) {
    return std::make_shared<ShaderProgram>(std::vector<ShaderSource>{
        makeVertexShader(loadFile("shaders/ui.vert.glsl")),
        makeFragmentShader(loadFile("shaders/ui.frag.glsl")),
    });
  }
};

struct UIFont {
  auto operator()(const Resources& resources) {
    return std::make_shared<Font>("fonts/calibri.ttf", 32);
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

struct RectNode {
  Mesh mesh;
  glm::vec4 color;
  RectNode(Mesh mesh, glm::vec4 color)
      : mesh(std::move(mesh)), color(std::move(color)) {}
};

// Builds the ready-to-render format of a rect node.
struct WorldRectNode {
  auto operator()(const Resources& resources, const std::string& id) {
    const auto& ui_node = resources.get<WorldUI>()->nodes.at(id);
    auto x = to<float>(ui_node.attr.at("x"));
    auto y = to<float>(ui_node.attr.at("y"));
    auto z = to<float>(get_or(ui_node.attr, "z", "1"));
    auto w = to<float>(ui_node.attr.at("width"));
    auto h = to<float>(ui_node.attr.at("height"));
    auto rgba = to<uint32_t>(ui_node.attr.at("color"));

    // Parse out the rect's geometry.
    Eigen::Matrix<float, 3, 6> positions;
    positions.row(0) << 0, w, w, w, 0, 0;
    positions.row(1) << 0, 0, h, h, h, 0;
    positions.row(2) << 0, 0, 0, 0, 0, 0;

    // Parse out the rect's color.
    auto color = glm::vec4(
        (rgba >> 24 & 0xFF) / 255.0f,
        (rgba >> 16 & 0xFF) / 255.0f,
        (rgba >> 8 & 0xFF) / 255.0f,
        (rgba & 0xFF) / 255.0f);

    return std::make_shared<RectNode>(
        MeshBuilder()
            .setPositions(std::move(positions))
            .setTransform(glm::translate(glm::mat4(1.0), glm::vec3(x, y, -z)))
            .build(),
        std::move(color));
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
    auto node_ids = resources_->get<WorldRectNodes>();
    for (const auto& node_id : *node_ids) {
      auto rect = resources_->get<WorldRectNode>(node_id);
      shader.uniform("use_texture", false);
      shader.uniform("model_matrix", rect->mesh.transform());
      shader.uniform("sprite_color", rect->color);
      rect->mesh.draw(shader);
    }
  }

 private:
  std::shared_ptr<Resources> resources_;
};

template <>
std::shared_ptr<RectUIRenderer> gen(const Registry& registry) {
  return std::make_shared<RectUIRenderer>(registry.get<Resources>());
}

// Builds the ready-to-render format of a text node.
struct WorldTextNode {
  auto operator()(const Resources& resources, const std::string& id) {
    const auto& ui_node = resources.get<WorldUI>()->nodes.at(id);
    auto x = to<float>(ui_node.attr.at("x"));
    auto y = to<float>(ui_node.attr.at("y"));
    auto z = to<float>(get_or(ui_node.attr, "z", "1"));
    auto rgba = to<uint32_t>(ui_node.attr.at("color"));

    // Build the text mesh.
    auto font = resources.get<UIFont>();
    auto text = font->buildText(ui_node.attr.at("text"));
    text.mesh.transform() = glm::translate(glm::mat4(1.0), glm::vec3(x, y, -z));

    // Parse out the text's color.
    auto color = glm::vec4(
        (rgba >> 24 & 0xFF) / 255.0f,
        (rgba >> 16 & 0xFF) / 255.0f,
        (rgba >> 8 & 0xFF) / 255.0f,
        (rgba & 0xFF) / 255.0f);

    return std::make_shared<Text>(
        std::move(text.mesh), std::move(text.texture), std::move(color));
  }
};

// A resource to index the IDs of all "text" nodes.
struct WorldTextNodes {
  std::shared_ptr<std::vector<std::string>> operator()(
      const Resources& resources) {
    auto ui = resources.get<WorldUI>();
    auto ret = std::make_shared<std::vector<std::string>>();
    for (const auto& pair : ui->nodes) {
      if (pair.second.kind == "text") {
        ret->push_back(pair.first);
      }
    }
    return ret;
  }
};

class TextUIRenderer {
 public:
  TextUIRenderer(std::shared_ptr<Resources> resources)
      : resources_(resources) {}
  void draw(ShaderProgram& shader) {
    auto node_ids = resources_->get<WorldTextNodes>();
    for (const auto& node_id : *node_ids) {
      auto text = resources_->get<WorldTextNode>(node_id);
      TextureBinding tb(*text->texture, 0);
      shader.uniform("sprite_map", tb.location());
      shader.uniform("use_texture", true);
      shader.uniform("model_matrix", text->mesh.transform());
      shader.uniform("sprite_color", text->color);
      text->mesh.draw(shader);
    }
  }

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
    auto ortho_mat = glm::ortho<float>(0.0, width, 0.0, height, 0.0, 1000.0);

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