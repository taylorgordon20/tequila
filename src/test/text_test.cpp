#include <iostream>

#include "src/common/images.hpp"
#include "src/common/text.hpp"

namespace tequila {

void run() {
  Font small_font("fonts/calibri.ttf", 19);

  // Generate an image (copy alpha into RGB for testing purposes).
  auto image_tensor = small_font.getAtlasImage();
  for (auto row = 0; row < image_tensor.dimension(0); row += 1) {
    for (auto col = 0; col < image_tensor.dimension(1); col += 1) {
      image_tensor(row, col, 0) = image_tensor(row, col, 3);
      image_tensor(row, col, 1) = image_tensor(row, col, 3);
      image_tensor(row, col, 2) = image_tensor(row, col, 3);
      image_tensor(row, col, 3) = 255;
    }
  }

  // Rasterize the atlas boxes into the image.
  auto atlas_index = small_font.getAtlasIndex();
  for (const auto& pair : atlas_index) {
    auto [x, y, w, h] = pair.second;
    for (int i = 0; i < w; i += 1) {
      image_tensor(y, x + i, 0) = 255;
      image_tensor(y + h - 1, x + i, 0) = 255;
    }
    for (int i = 0; i < h; i += 1) {
      image_tensor(y + i, x, 0) = 255;
      image_tensor(y + i, x + w - 1, 0) = 255;
    }
  }

  saveTensorToPng("text_test.png", image_tensor);
  std::cout << "Saved image: text_test.png" << std::endl;
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}