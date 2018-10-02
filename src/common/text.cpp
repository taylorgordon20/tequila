#include "src/common/text.hpp"

#include <ft2build.h>
#include <Eigen/Dense>
#include FT_FREETYPE_H

#include <codecvt>
#include <locale>
#include <memory>
#include <tuple>
#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/images.hpp"
#include "src/common/meshes.hpp"
#include "src/common/opengl.hpp"
#include "src/common/textures.hpp"

namespace tequila {

namespace {

constexpr auto kInitialAtlasSize = 256;

FT_Face loadFreeTypeFace(const std::string& filename, size_t size) {
  static FT_Library library = [] {
    FT_Library ret;
    ENFORCE(!FT_Init_FreeType(&ret));
    return ret;
  }();

  FT_Face face;
  ENFORCE(!FT_New_Face(library, filename.c_str(), 0, &face));
  ENFORCE(!FT_Set_Pixel_Sizes(face, 0, size));
  return face;
}

}  // anonymous namespace

Font::Font(const char* font_file, size_t font_size)
    : font_face_(loadFreeTypeFace(resolvePathOrThrow(font_file), font_size)),
      atlas_size_(kInitialAtlasSize) {
  // Initialize the index with ASCII codepoints.
  for (char32_t c = 32; c < 127; c += 1) {
    atlas_index_.emplace(c, std::tuple(0, 0, 0, 0));
  }
  buildAtlas();
}

Text Font::buildText(const std::string& text) {
#if _MSC_VER >= 1900
  std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> converter;
  auto int32_string = converter.from_bytes(text);
  return buildText(std::u32string(
      reinterpret_cast<const char32_t*>(int32_string.data()),
      int32_string.size()));
#else
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  return buildText(converter.from_bytes(text));
#endif
}

Text Font::buildText(const std::u32string& text) {
  // Make sure every character is in the texture atlas.
  bool build_required = false;
  for (char32_t c : text) {
    if (!atlas_index_.count(c)) {
      atlas_index_.emplace(c, std::tuple(0, 0, 0, 0));
      build_required = true;
    }
  }
  if (build_required) {
    buildAtlas();
  }

  // Build the mesh data.
  int offset = 0;
  Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 6 * text.size());
  Eigen::Matrix<float, 2, Eigen::Dynamic> tex_coords(2, 6 * text.size());
  for (int i = 0; i < text.size(); i += 1) {
    static Eigen::Matrix<float, 3, 6> kPositions = [] {
      Eigen::Matrix<float, 3, 6> ret;
      ret.row(0) << 0, 1, 1, 1, 0, 0;
      ret.row(1) << 0, 0, 1, 1, 1, 0;
      ret.row(2) << 0, 0, 0, 0, 0, 0;
      return ret;
    }();
    static Eigen::Matrix<float, 2, 6> kTexCoords = [] {
      Eigen::Matrix<float, 2, 6> ret;
      ret.row(0) << 0, 1, 1, 1, 0, 0;
      ret.row(1) << 0, 0, 1, 1, 1, 0;
      return ret;
    }();
    static auto kOnesRow = Eigen::Matrix<float, 1, 6>::Ones();

    auto [x, y, w, h] = atlas_index_.at(text.at(i));

    // Set position based on the size of the character and the current offset.
    positions.block(0, 6 * i, 3, 6) = kPositions;
    positions.row(0).segment(6 * i, 6) *= w;
    positions.row(1).segment(6 * i, 6) *= h;
    positions.row(0).segment(6 * i, 6) += offset * kOnesRow;

    // Set the texture coordinates based on the atlas rect.
    tex_coords.block(0, 6 * i, 2, 6) = kTexCoords;
    tex_coords.row(0).segment(6 * i, 6) *= w;
    tex_coords.row(1).segment(6 * i, 6) *= h;
    tex_coords.row(0).segment(6 * i, 6) += x * kOnesRow;
    tex_coords.row(1).segment(6 * i, 6) += y * kOnesRow;
    tex_coords.block(0, 6 * i, 2, 6) /= atlas_size_;

    offset += w;
  }

  return Text(
      MeshBuilder()
          .setPositions(std::move(positions))
          .setTexCoords(std::move(tex_coords))
          .build(),
      getTexture(),
      glm::vec4(1.0, 1.0, 1.0, 1.0));
}

void Font::buildAtlas() {
  std::vector<char32_t> codepoints;
  for (const auto& pair : atlas_index_) {
    codepoints.push_back(pair.first);
  }

  // Rasterize each character into the pixels array.
  atlas_pixels_ = ImageTensor(atlas_size_, atlas_size_, 4);
  atlas_pixels_.setZero();
  int row_offset = 0;
  int col_offset = 0;
  for (int i = 0; i < codepoints.size(); i += 1) {
    // Render the character's glyph image as a bitmap.
    FT_GlyphSlot slot = font_face_->glyph;
    FT_UInt glyph_index = FT_Get_Char_Index(font_face_, codepoints.at(i));
    ENFORCE(!FT_Load_Glyph(font_face_, glyph_index, FT_LOAD_DEFAULT));
    ENFORCE(!FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL));

    int advance_width = slot->advance.x >> 6;
    int advance_height = font_face_->height >> 6;

    // Wrap to a new row if necessary.
    if (col_offset + advance_width > atlas_size_) {
      ENFORCE(col_offset > 0);
      col_offset = 0;
      row_offset += advance_height;
    }

    // If we've run out of space, double the size of the texture and start over.
    if (row_offset + advance_height > atlas_size_) {
      std::cout << "ran out of space" << std::endl;
      atlas_size_ = atlas_size_ << 1;
      atlas_pixels_ = ImageTensor(atlas_size_, atlas_size_, 4);
      atlas_pixels_.setZero();
      row_offset = 0;
      col_offset = 0;
      i = -1;
      continue;
    }

    // Copy the bitmap into the texture pixels.
    for (int row = 0; row < slot->bitmap.rows; row += 1) {
      for (int col = 0; col < slot->bitmap.width; col += 1) {
        int baseline = advance_height - (font_face_->ascender >> 6);
        int s_y = row_offset + baseline + slot->bitmap_top;
        int s_x = col_offset + slot->bitmap_left;
        uint8_t alpha = slot->bitmap.buffer[row * slot->bitmap.width + col];
        atlas_pixels_(s_y - row - 1, s_x + col, 0) = 255;
        atlas_pixels_(s_y - row - 1, s_x + col, 1) = 255;
        atlas_pixels_(s_y - row - 1, s_x + col, 2) = 255;
        atlas_pixels_(s_y - row - 1, s_x + col, 3) = alpha;
      }
    }

    // Update the atlas.
    atlas_index_[codepoints.at(i)] =
        std::make_tuple(col_offset, row_offset, advance_width, advance_height);

    // Update offsets.
    col_offset += advance_width;
  }
}

std::shared_ptr<Texture> Font::getTexture() {
  if (!texture_) {
    texture_ = std::make_shared<Texture>(atlas_pixels_);
  }
  return texture_;
}

const ImageTensor& Font::getAtlasImage() const {
  return atlas_pixels_;
}

const AtlasIndex& Font::getAtlasIndex() const {
  return atlas_index_;
}

}  // namespace tequila