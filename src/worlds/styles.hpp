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

struct TerrainStyleConfig {
  std::string name;
  std::string color;
  std::string color_map;
  std::string normal_map;

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
       cereal::make_nvp("normal_map", normal_map));
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
  auto operator()(const ResourceDeps& deps) {
    std::stringstream ss;
    ss << loadFile("configs/terrain.json");
    cereal::JSONInputArchive archive(ss);
    auto style_config = std::make_shared<TerrainStyleIndex>();
    style_config->serialize(archive);
    return style_config;
  }
};

struct TerrainStylesColorMapIndex {
  std::unordered_map<int64_t, int> index;
  std::shared_ptr<TextureArray> texture_array;
  TerrainStylesColorMapIndex(
      std::unordered_map<int64_t, int> index,
      std::shared_ptr<TextureArray> texture_array)
      : index(std::move(index)), texture_array(std::move(texture_array)) {}
};

struct TerrainStylesColorMap {
  auto operator()(const ResourceDeps& deps) {
    // Build the color map index.
    std::vector<std::string> color_maps;
    std::unordered_map<int64_t, int> style_index;
    std::unordered_map<std::string, int> color_map_index;
    for (const auto& style_pair : deps.get<TerrainStyles>()->styles) {
      const auto& color_map = style_pair.second.color_map;
      if (!color_map_index.count(color_map)) {
        color_map_index[color_map] = color_maps.size();
        color_maps.push_back(color_map);
      }
      style_index[style_pair.first] = color_map_index.at(color_map);
    }

    // Load the pixel data for each color map.
    std::vector<ImageTensor> pixels;
    for (const auto& color_map : color_maps) {
      pixels.push_back(loadPngToTensor(color_map));
    }

    return std::make_shared<TerrainStylesColorMapIndex>(
        std::move(style_index), std::make_shared<TextureArray>(pixels));
  }
};

struct TerrainStylesNormalMapIndex {
  std::unordered_map<int64_t, int> index;
  std::shared_ptr<TextureArray> texture_array;
  TerrainStylesNormalMapIndex(
      std::unordered_map<int64_t, int> index,
      std::shared_ptr<TextureArray> texture_array)
      : index(std::move(index)), texture_array(std::move(texture_array)) {}
};

struct TerrainStylesNormalMap {
  auto operator()(const ResourceDeps& deps) {
    // Build the normal map index.
    std::vector<std::string> normal_maps;
    std::unordered_map<int64_t, int> style_index;
    std::unordered_map<std::string, int> normal_map_index;
    for (const auto& style_pair : deps.get<TerrainStyles>()->styles) {
      const auto& normal_map = style_pair.second.normal_map;
      if (!normal_map_index.count(normal_map)) {
        normal_map_index[normal_map] = normal_maps.size();
        normal_maps.push_back(normal_map);
      }
      style_index[style_pair.first] = normal_map_index.at(normal_map);
    }

    // Load the pixel data for each normal map.
    std::vector<ImageTensor> pixels;
    for (const auto& normal_map : normal_maps) {
      pixels.push_back(loadPngToTensor(normal_map));
    }

    return std::make_shared<TerrainStylesNormalMapIndex>(
        std::move(style_index), std::make_shared<TextureArray>(pixels));
  }
};

}  // namespace tequila