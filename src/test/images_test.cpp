#include <iostream>

#include "src/common/images.hpp"

namespace tequila {

void run() {
  constexpr auto kInputImagePath = "images/spatial_test.png";
  constexpr auto kOutputImagePath = "image_test.png";

  // Read pixels from disk.
  auto raw_pixels = loadPngToTensor(kInputImagePath);
  std::cout << "Loaded image: " << kInputImagePath << std::endl;

  // Convert pixels to normalized float representation.
  Eigen::Tensor<float, 3> pixels = raw_pixels.cast<float>() / 255.0f;

  // Convert image to only have the blue channel.
  pixels.chip<2>(0).setZero();
  pixels.chip<2>(1).setZero();

  // Convert pixels back to byte format and save them to disk.
  raw_pixels = (pixels * 255.0f + 0.5f).cast<uint8_t>();
  saveTensorToPng("image_test.png", raw_pixels);
  std::cout << "Saved image: " << kOutputImagePath << std::endl;
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}