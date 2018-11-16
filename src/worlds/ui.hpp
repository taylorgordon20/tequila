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
#include "src/worlds/styles.hpp"

namespace tequila {

struct UIFont {
  auto operator()(ResourceDeps& deps, const std::string& style, size_t size) {
    return std::make_shared<Font>(format("fonts/%1%.ttf", style), size);
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
  auto operator()(ResourceDeps& deps, const std::string& id) {
    StatsTimer timer(registryGet<Stats>(deps), "ui.rect_node");

    auto ui_tree = deps.get<WorldUI>();
    const auto& ui_node = ui_tree->nodes.at(id);
    auto x = to<float>(get_or(ui_node.attr, "x", "0"));
    auto y = to<float>(get_or(ui_node.attr, "y", "0"));
    auto z = to<float>(get_or(ui_node.attr, "z", "1"));
    auto w = to<float>(get_or(ui_node.attr, "width", "0"));
    auto h = to<float>(get_or(ui_node.attr, "height", "0"));
    auto rgba = to<uint32_t>(get_or(ui_node.attr, "color", "0"));

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

    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new RectNode(
          MeshBuilder()
              .setPositions(std::move(positions))
              .setTransform(glm::translate(glm::mat4(1.0), glm::vec3(x, y, -z)))
              .build(),
          std::move(color));
    });
  }
};

// A resource to index the IDs of all "rect" nodes.
struct WorldRectNodes {
  std::shared_ptr<std::vector<std::string>> operator()(ResourceDeps& deps) {
    auto ui = deps.get<WorldUI>();
    auto ret = std::make_shared<std::vector<std::string>>();
    for (const auto& pair : ui->nodes) {
      if (pair.second.kind == "rect") {
        ret->push_back(pair.first);
      }
    }
    return ret;
  }
};

struct RectUIShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/ui.vert.glsl")),
          makeFragmentShader(loadFile("shaders/ui.rect.frag.glsl")),
      });
    });
  }
};

class RectUIRenderer {
 public:
  RectUIRenderer(std::shared_ptr<AsyncResources> resources)
      : resources_(resources) {}
  void draw(const glm::mat4& projection) {
    auto shader = resources_->syncGet<RectUIShader>();
    auto node_ids = resources_->syncGet<WorldRectNodes>();
    shader->run([&] {
      shader->uniform("projection_matrix", projection);
      for (const auto& node_id : *node_ids) {
        if (auto opt_rect = resources_->optGet<WorldRectNode>(node_id)) {
          auto rect = opt_rect.get();
          shader->uniform("model_matrix", rect->mesh.transform());
          shader->uniform("base_color", rect->color);
          rect->mesh.draw(*shader);
        }
      }
    });
  }

 private:
  std::shared_ptr<AsyncResources> resources_;
};

template <>
std::shared_ptr<RectUIRenderer> gen(const Registry& registry) {
  return std::make_shared<RectUIRenderer>(registry.get<AsyncResources>());
}

// Builds the ready-to-render format of a text node.
struct WorldTextNode {
  auto operator()(ResourceDeps& deps, const std::string& id) {
    StatsTimer timer(registryGet<Stats>(deps), "ui.text_node");

    auto ui_tree = deps.get<WorldUI>();
    const auto& ui_node = ui_tree->nodes.at(id);
    auto x = to<float>(get_or(ui_node.attr, "x", "0"));
    auto y = to<float>(get_or(ui_node.attr, "y", "0"));
    auto z = to<float>(get_or(ui_node.attr, "z", "1"));
    auto rgba = to<uint32_t>(get_or(ui_node.attr, "color", "0"));
    auto size = to<size_t>(get_or(ui_node.attr, "size", "20"));
    auto font = get_or(ui_node.attr, "font", "Roboto/Roboto-Regular");
    auto text = get_or(ui_node.attr, "text", "");

    // Parse out the text's color.
    auto color = glm::vec4(
        (rgba >> 24 & 0xFF) / 255.0f,
        (rgba >> 16 & 0xFF) / 255.0f,
        (rgba >> 8 & 0xFF) / 255.0f,
        (rgba & 0xFF) / 255.0f);

    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      // Build the text mesh.
      auto node = deps.get<UIFont>(font, size)->buildText(text);
      node.mesh.transform() =
          glm::translate(glm::mat4(1.0), glm::vec3(x, y, -z));
      return new Text(
          std::move(node.mesh), std::move(node.texture), std::move(color));
    });
  }
};

// A resource to index the IDs of all "text" nodes.
struct WorldTextNodes {
  std::shared_ptr<std::vector<std::string>> operator()(ResourceDeps& deps) {
    auto ui = deps.get<WorldUI>();
    auto ret = std::make_shared<std::vector<std::string>>();
    for (const auto& pair : ui->nodes) {
      if (pair.second.kind == "text") {
        ret->push_back(pair.first);
      }
    }
    return ret;
  }
};

struct TextUIShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/ui.vert.glsl")),
          makeFragmentShader(loadFile("shaders/ui.text.frag.glsl")),
      });
    });
  }
};

class TextUIRenderer {
 public:
  TextUIRenderer(std::shared_ptr<AsyncResources> resources)
      : resources_(resources) {}
  void draw(const glm::mat4& projection) {
    auto shader = resources_->syncGet<TextUIShader>();
    auto node_ids = resources_->syncGet<WorldTextNodes>();
    shader->run([&] {
      shader->uniform("projection_matrix", projection);
      for (const auto& node_id : *node_ids) {
        if (auto opt_text = resources_->optGet<WorldTextNode>(node_id)) {
          auto text = opt_text.get();
          TextureBinding tb(*text->texture, 0);
          shader->uniform("color_map", tb.location());
          shader->uniform("model_matrix", text->mesh.transform());
          shader->uniform("base_color", text->color);
          text->mesh.draw(*shader);
        }
      }
    });
  }

 private:
  std::shared_ptr<AsyncResources> resources_;
};

template <>
inline std::shared_ptr<TextUIRenderer> gen(const Registry& registry) {
  return std::make_shared<TextUIRenderer>(registry.get<AsyncResources>());
}

struct StyleNode {
  Mesh mesh;
  glm::vec4 color;
  int color_map_index;
  int normal_map_index;
  std::shared_ptr<TextureArray> color_map;
  std::shared_ptr<TextureArray> normal_map;

  StyleNode(
      Mesh mesh,
      glm::vec4 color,
      int color_map_index,
      int normal_map_index,
      std::shared_ptr<TextureArray> color_map,
      std::shared_ptr<TextureArray> normal_map)
      : mesh(std::move(mesh)),
        color(std::move(color)),
        color_map_index(color_map_index),
        normal_map_index(normal_map_index),
        color_map(std::move(color_map)),
        normal_map(std::move(normal_map)) {}
};

// Builds the ready-to-render format of a style node.
struct WorldStyleNode {
  auto operator()(ResourceDeps& deps, const std::string& id) {
    StatsTimer timer(registryGet<Stats>(deps), "ui.style_node");

    auto ui_tree = deps.get<WorldUI>();
    const auto& ui_node = ui_tree->nodes.at(id);
    auto x = to<float>(get_or(ui_node.attr, "x", "0"));
    auto y = to<float>(get_or(ui_node.attr, "y", "0"));
    auto z = to<float>(get_or(ui_node.attr, "z", "1"));
    auto w = to<float>(get_or(ui_node.attr, "width", "0"));
    auto h = to<float>(get_or(ui_node.attr, "height", "0"));
    auto style = to<uint32_t>(get_or(ui_node.attr, "style", "1"));
    auto rgba = to<uint32_t>(get_or(ui_node.attr, "color", "0"));

    // Parse out the style data.
    auto terrain_styles = deps.get<TerrainStyles>();
    auto color_maps = deps.get<TerrainStylesColorMap>();
    auto normal_maps = deps.get<TerrainStylesNormalMap>();

    // Parse out the rect's geometry.
    Eigen::Matrix<float, 3, 6> positions;
    positions.row(0) << 0, w, w, w, 0, 0;
    positions.row(1) << 0, 0, h, h, h, 0;
    positions.row(2) << 0, 0, 0, 0, 0, 0;

    // Parse out the rect's color.
    auto color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    if (auto style_ptr = get_ptr(terrain_styles->styles, style)) {
      color = style_ptr->colorVec();
    }
    color[0] *= (rgba >> 24 & 0xFF) / 255.0f;
    color[1] *= (rgba >> 16 & 0xFF) / 255.0f;
    color[2] *= (rgba >> 8 & 0xFF) / 255.0f;
    color[3] *= (rgba & 0xFF) / 255.0f;

    // Parse out the texture coordinates.
    Eigen::Matrix<float, 2, 6> tex_coords;
    tex_coords.row(0) << 0, 1, 1, 1, 0, 0;
    tex_coords.row(1) << 0, 0, 1, 1, 1, 0;

    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new StyleNode(
          MeshBuilder()
              .setPositions(std::move(positions))
              .setTexCoords(std::move(tex_coords))
              .setTransform(glm::translate(glm::mat4(1.0), glm::vec3(x, y, -z)))
              .build(),
          std::move(color),
          color_maps->indexOrDefault(StyleIndexKey(style, "top")),
          normal_maps->indexOrDefault(StyleIndexKey(style, "top")),
          color_maps->texture_array,
          normal_maps->texture_array);
    });
  }
};

// A resource to index the IDs of all "style" nodes.
struct WorldStyleNodes {
  std::shared_ptr<std::vector<std::string>> operator()(ResourceDeps& deps) {
    auto ui = deps.get<WorldUI>();
    auto ret = std::make_shared<std::vector<std::string>>();
    for (const auto& pair : ui->nodes) {
      if (pair.second.kind == "style") {
        ret->push_back(pair.first);
      }
    }
    return ret;
  }
};

struct StyleUIShader {
  auto operator()(ResourceDeps& deps) {
    return registryGet<OpenGLContextExecutor>(deps)->manage([&] {
      return new ShaderProgram(std::vector<ShaderSource>{
          makeVertexShader(loadFile("shaders/ui.vert.glsl")),
          makeFragmentShader(loadFile("shaders/ui.style.frag.glsl")),
      });
    });
  }
};

class StyleUIRenderer {
 public:
  StyleUIRenderer(std::shared_ptr<AsyncResources> resources)
      : resources_(resources) {}
  void draw(const glm::mat4& projection) {
    auto shader = resources_->syncGet<StyleUIShader>();
    auto node_ids = resources_->syncGet<WorldStyleNodes>();
    shader->run([&] {
      shader->uniform("projection_matrix", projection);
      for (const auto& node_id : *node_ids) {
        if (auto opt_node = resources_->optGet<WorldStyleNode>(node_id)) {
          auto node = opt_node.get();
          TextureArrayBinding color_map_array(*node->color_map, 0);
          TextureArrayBinding normal_map_array(*node->normal_map, 1);
          shader->uniform("color_map_array", color_map_array.location());
          shader->uniform("color_map_array_index", node->color_map_index);
          shader->uniform("normal_map_array", normal_map_array.location());
          shader->uniform("normal_map_array_index", node->normal_map_index);
          shader->uniform("base_color", node->color);
          shader->uniform("model_matrix", node->mesh.transform());
          node->mesh.draw(*shader);
        }
      }
    });
  }

 private:
  std::shared_ptr<AsyncResources> resources_;
};

template <>
inline std::shared_ptr<StyleUIRenderer> gen(const Registry& registry) {
  return std::make_shared<StyleUIRenderer>(registry.get<AsyncResources>());
}

class UIRenderer {
 public:
  UIRenderer(
      std::shared_ptr<Stats> stats,
      std::shared_ptr<Window> window,
      std::shared_ptr<Resources> resources,
      std::shared_ptr<RectUIRenderer> rect_renderer,
      std::shared_ptr<TextUIRenderer> text_renderer,
      std::shared_ptr<StyleUIRenderer> style_renderer)
      : stats_(stats),
        window_(window),
        resources_(resources),
        rect_renderer_(rect_renderer),
        text_renderer_(text_renderer),
        style_renderer_(style_renderer) {}

  void draw() {
    StatsTimer loop_timer(stats_, "ui_renderer");

    // Prepare OpenGL state.
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glEnable(gl::GL_BLEND);
    gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
    Finally finally([] {
      gl::glDisable(gl::GL_BLEND);
      gl::glEnable(gl::GL_DEPTH_TEST);
    });

    // Build the orthographic projection martix.
    int width, height;
    window_->call<glfwGetFramebufferSize>(&width, &height);
    auto ortho_mat = glm::ortho<float>(0.0, width, 0.0, height, 0.0, 1000.0);

    // Invoke UI renderers.
    rect_renderer_->draw(ortho_mat);
    text_renderer_->draw(ortho_mat);
    style_renderer_->draw(ortho_mat);
  }

 private:
  std::shared_ptr<Stats> stats_;
  std::shared_ptr<Window> window_;
  std::shared_ptr<Resources> resources_;
  std::shared_ptr<RectUIRenderer> rect_renderer_;
  std::shared_ptr<TextUIRenderer> text_renderer_;
  std::shared_ptr<StyleUIRenderer> style_renderer_;
};

template <>
inline std::shared_ptr<UIRenderer> gen(const Registry& registry) {
  return std::make_shared<UIRenderer>(
      registry.get<Stats>(),
      registry.get<Window>(),
      registry.get<Resources>(),
      registry.get<RectUIRenderer>(),
      registry.get<TextUIRenderer>(),
      registry.get<StyleUIRenderer>());
}

}  // namespace tequila