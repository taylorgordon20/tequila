#pragma once

#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/gil_all.hpp>
#include <unsupported/Eigen/CXX11/Tensor>

#include "src/common/files.hpp"

namespace tequila {

inline auto loadPng(const std::string& path) {
  boost::gil::rgb8_image_t image;
  boost::gil::read_and_convert_image(
      resolvePathOrThrow(path).c_str(), image, boost::gil::png_tag());
  return image;
}

inline void savePng(
    const std::string& path, const boost::gil::rgb8_image_t& image) {
  boost::gil::write_view(
      path.c_str(), const_view(image), boost::gil::png_tag());
}

using ImageTensor = Eigen::Tensor<uint8_t, 3, Eigen::RowMajor>;

inline auto loadPngToTensor(const std::string& path) {
  auto image = loadPng(path);
  auto image_view = const_view(image);
  ImageTensor pixels(image.height(), image.width(), 3);
  for (auto row = 0; row < image.height(); row += 1) {
    for (auto col = 0; col < image.width(); col += 1) {
      auto inverted_row = image.height() - row - 1;
      pixels(row, col, 0) = boost::gil::at_c<0>(image_view(col, inverted_row));
      pixels(row, col, 1) = boost::gil::at_c<1>(image_view(col, inverted_row));
      pixels(row, col, 2) = boost::gil::at_c<2>(image_view(col, inverted_row));
    }
  }
  return pixels;
}

inline auto saveTensorToPng(
    const std::string& path, const ImageTensor& tensor) {
  boost::gil::rgb8_image_t image(tensor.dimension(1), tensor.dimension(0));
  auto image_view = view(image);
  for (auto row = 0; row < image.height(); row += 1) {
    for (auto col = 0; col < image.height(); col += 1) {
      auto inverted_row = image.height() - row - 1;
      image_view(col, inverted_row) = boost::gil::rgb8_pixel_t(
          tensor(row, col, 0), tensor(row, col, 1), tensor(row, col, 2));
    }
  }
  savePng(path, image);
}

inline auto subImage(
    const ImageTensor& tensor, int x, int y, size_t w, size_t h) {
  Eigen::DSizes<ptrdiff_t, 3> offset_slice(y, x, 0);
  Eigen::DSizes<ptrdiff_t, 3> size_slice(h, w, tensor.dimension(2));
  return tensor.slice(offset_slice, size_slice);
}

inline auto invertY(const ImageTensor& tensor) {
  auto h = tensor.dimension(0);
  auto w = tensor.dimension(1);
  auto d = tensor.dimension(2);
  ImageTensor ret(h, w, d);
  for (int row = 0; row < h; row += 1) {
    ret.chip(row, 0) = tensor.chip(h - row - 1, 0);
  }
  return ret;
}

}  // namespace tequila
