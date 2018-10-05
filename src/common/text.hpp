#pragma once

#ifdef _WIN32
#include "windows.h"
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>

#include <memory>
#include <tuple>
#include <unordered_map>

#include "src/common/meshes.hpp"
#include "src/common/opengl.hpp"
#include "src/common/textures.hpp"

namespace tequila {

struct Text {
  Mesh mesh;
  std::shared_ptr<Texture> texture;
  glm::vec4 color;
  Text(Mesh mesh, std::shared_ptr<Texture> texture, glm::vec4 color)
      : mesh(std::move(mesh)),
        texture(std::move(texture)),
        color(std::move(color)) {}
};

using AtlasIndex = std::unordered_map<char32_t, std::tuple<int, int, int, int>>;

class Font {
 public:
  Font(const std::string& font_file, size_t font_size);
  Text buildText(const std::string& text);
  Text buildText(const std::u32string& text);
  std::shared_ptr<Texture> getTexture();
  const ImageTensor& getAtlasImage() const;
  const AtlasIndex& getAtlasIndex() const;

 private:
  void buildAtlas();

  FT_Face font_face_;
  size_t atlas_size_;
  ImageTensor atlas_pixels_;
  AtlasIndex atlas_index_;
  std::shared_ptr<Texture> texture_;
};

}  // namespace tequila