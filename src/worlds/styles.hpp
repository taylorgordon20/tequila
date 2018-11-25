#pragma once

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/images.hpp"
#include "src/common/resources.hpp"
#include "src/common/textures.hpp"
#include "src/worlds/core.hpp"

namespace tequila {

struct WorldStyleName
    : public SeedResource<WorldStyleName, std::shared_ptr<std::string>> {};

static const std::vector<std::string> kStyleOverrideKeys = {
    "left",
    "right",
    "bottom",
    "top",
    "back",
    "front",
};

struct TerrainStyleConfig {
  std::string name;
  std::string color;
  std::string color_map;
  std::string normal_map;
  std::unordered_map<std::string, std::string> color_map_overrides;
  std::unordered_map<std::string, std::string> normal_map_overrides;

  auto colorVec() const {
    uint32_t rgb = std::stoul(color, nullptr, 16);
    return glm::vec4(
        ((rgb >> 16) & 0xFF) / 255.0f,
        ((rgb >> 8) & 0xFF) / 255.0f,
        (rgb & 0xFF) / 255.0f,
        1.0f);
  }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar(cereal::make_nvp("name", name),
       cereal::make_nvp("color", color),
       cereal::make_nvp("color_map", color_map),
       cereal::make_nvp("normal_map", normal_map),
       cereal::make_nvp("color_map_overrides", color_map_overrides),
       cereal::make_nvp("normal_map_overrides", normal_map_overrides));
  }
};

struct TerrainStyleIndex {
  std::unordered_map<int64_t, TerrainStyleConfig> styles;

  template <typename Archive>
  void serialize(Archive& ar) {
    ar(cereal::make_nvp("styles", styles));
  }
};

struct TerrainStyles {
  auto operator()(ResourceDeps& deps) {
    std::stringstream ss;
    auto config_name = deps.get<WorldStyleName>();
    ss << loadFile(format("configs/%1%.json", *config_name));
    cereal::JSONInputArchive archive(ss);
    auto style_config = std::make_shared<TerrainStyleIndex>();
    style_config->serialize(archive);
    return style_config;
  }
};

using StyleIndexKey = std::tuple<int64_t, std::string>;

struct StyleIndexKeyHash {
  std::size_t operator()(const StyleIndexKey& key) const noexcept {
    return boost::hash_value(key);
  }
};

using StyleIndexMap = std::unordered_map<StyleIndexKey, int, StyleIndexKeyHash>;

struct TerrainStylesColorMapIndex {
  StyleIndexMap index;
  std::shared_ptr<TextureArray> texture_array;

  TerrainStylesColorMapIndex(
      StyleIndexMap index, std::shared_ptr<TextureArray> texture_array)
      : index(std::move(index)), texture_array(std::move(texture_array)) {}

  auto indexOrDefault(const StyleIndexKey& key) {
    const StyleIndexKey kDefaultKey(1, "top");
    return get_or(index, key, index.at(kDefaultKey));
  }

  auto indexOrDefault(int64_t key, const std::string& override_key) {
    return indexOrDefault(StyleIndexKey(key, override_key));
  }
};

struct TerrainStylesColorMap {
  auto operator()(ResourceDeps& deps) {
    StatsTimer timer(deps.get<WorldStats>(), "terrain_color_styles");
    auto terrain_styles = deps.get<TerrainStyles>();

    // Build the color map index.
    StyleIndexMap style_index;
    std::vector<std::string> color_maps;
    std::unordered_map<std::string, int> color_map_index;
    for (const auto& style_pair : terrain_styles->styles) {
      const auto& style_config = style_pair.second;
      for (const auto& override_key : kStyleOverrideKeys) {
        const auto& color_map = get_or(
            style_config.color_map_overrides,
            override_key,
            style_config.color_map);

        // Allocate an index for this color map on first encounter.
        if (!color_map_index.count(color_map)) {
          color_map_index[color_map] = color_maps.size();
          color_maps.push_back(color_map);
        }

        // Point this style and override key to the color map index.
        auto style_key = StyleIndexKey(style_pair.first, override_key);
        style_index[style_key] = color_map_index.at(color_map);
      }
    }

    // Load the pixel data for each color map.
    std::vector<ImageTensor> pixels;
    for (const auto& color_map : color_maps) {
      pixels.push_back(loadPngToTensor(color_map));
    }

    return deps.get<OpenGLExecutor>()->manage([&] {
      return new TerrainStylesColorMapIndex(
          std::move(style_index), std::make_shared<TextureArray>(pixels));
    });
  }
};

struct TerrainStylesNormalMapIndex {
  StyleIndexMap index;
  std::shared_ptr<TextureArray> texture_array;

  TerrainStylesNormalMapIndex(
      StyleIndexMap index, std::shared_ptr<TextureArray> texture_array)
      : index(std::move(index)), texture_array(std::move(texture_array)) {}

  auto indexOrDefault(const StyleIndexKey& key) {
    const StyleIndexKey kDefaultKey(1, "top");
    return get_or(index, key, index.at(kDefaultKey));
  }

  auto indexOrDefault(int64_t key, const std::string& override_key) {
    return indexOrDefault(StyleIndexKey(key, override_key));
  }
};

struct TerrainStylesNormalMap {
  auto operator()(ResourceDeps& deps) {
    StatsTimer timer(deps.get<WorldStats>(), "terrain_normal_styles");
    auto terrain_styles = deps.get<TerrainStyles>();

    // Build the normal map index.
    StyleIndexMap style_index;
    std::vector<std::string> normal_maps;
    std::unordered_map<std::string, int> normal_map_index;
    for (const auto& style_pair : terrain_styles->styles) {
      const auto& style_config = style_pair.second;
      for (const auto& override_key : kStyleOverrideKeys) {
        const auto& normal_map = get_or(
            style_config.normal_map_overrides,
            override_key,
            style_config.normal_map);

        // Allocate an index for this normal map on first encounter.
        if (!normal_map_index.count(normal_map)) {
          normal_map_index[normal_map] = normal_maps.size();
          normal_maps.push_back(normal_map);
        }

        // Point this style and override key to the normal map index.
        auto style_key = StyleIndexKey(style_pair.first, override_key);
        style_index[style_key] = normal_map_index.at(normal_map);
      }
    }

    // Load the pixel data for each normal map.
    std::vector<ImageTensor> pixels;
    for (const auto& normal_map : normal_maps) {
      pixels.push_back(loadPngToTensor(normal_map));
    }

    return deps.get<OpenGLExecutor>()->manage([&] {
      return new TerrainStylesNormalMapIndex(
          std::move(style_index), std::make_shared<TextureArray>(pixels));
    });
  }
};

}  // namespace tequila