#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/gil_all.hpp>
#include <unsupported/Eigen/CXX11/Tensor>

#include "src/common/files.hpp"

namespace tequila {

inline auto loadPng(const std::string& path) {
  boost::gil::rgb8_image_t image;
  boost::gil::read_and_convert_image(
      resolvePathOrThrow(path).string().c_str(), image, boost::gil::png_tag());
  return image;
}

inline void savePng(
    const std::string& path, const boost::gil::rgb8_image_t& image) {
  boost::gil::write_view(
      path.c_str(), const_view(image), boost::gil::png_tag());
}

inline auto loadPngToTensor(const std::string& path) {
  auto image = loadPng(path);
  auto image_view = const_view(image);
  Eigen::Tensor<uint8_t, 3> pixels(image.height(), image.width(), 3);
  for (auto row = 0; row < image.height(); row += 1) {
    for (auto col = 0; col < image.width(); col += 1) {
      pixels(row, col, 0) = boost::gil::at_c<0>(image_view(row, col));
      pixels(row, col, 1) = boost::gil::at_c<1>(image_view(row, col));
      pixels(row, col, 2) = boost::gil::at_c<2>(image_view(row, col));
    }
  }
  return pixels;
}

inline auto saveTensorToPng(
    const std::string& path, const Eigen::Tensor<uint8_t, 3>& tensor) {
  boost::gil::rgb8_image_t image(tensor.dimension(1), tensor.dimension(0));
  auto image_view = view(image);
  for (auto row = 0; row < image.height(); row += 1) {
    for (auto col = 0; col < image.height(); col += 1) {
      image_view(row, col) = boost::gil::rgb8_pixel_t(
          tensor(row, col, 0), tensor(row, col, 1), tensor(row, col, 2));
    }
  }
  savePng(path, image);
}

}  // namespace tequila
