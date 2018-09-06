#include "src/common/voxels.hpp"

#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/meshes.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

static constexpr size_t kVoxelDimSize = 1024;

namespace {
template <int cols>
auto positionMat(const std::vector<int>& indices) {
  ENFORCE(indices.size() == cols);
  static std::vector<std::tuple<float, float, float>> positions = {
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, 1.0f},
      {1.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f},
      {0.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 1.0f},
      {1.0f, 1.0f, 0.0f},
  };
  Eigen::Matrix<float, 3, cols> mat;
  for (int i = 0; i < cols; i += 1) {
    ENFORCE(0 <= indices.at(i) && indices.at(i) < positions.size());
    const auto& position = positions.at(indices.at(i));
    mat(0, i) = std::get<0>(position);
    mat(1, i) = std::get<1>(position);
    mat(2, i) = std::get<2>(position);
  }
  return mat;
}

template <int cols>
auto normalMat(std::tuple<float, float, float> normal) {
  Eigen::Matrix<float, 3, cols> mat;
  for (int i = 0; i < cols; i += 1) {
    mat(0, i) = std::get<0>(normal);
    mat(1, i) = std::get<1>(normal);
    mat(2, i) = std::get<2>(normal);
  }
  return mat;
}
}  // anonymous namespace

VoxelArray::VoxelArray() : voxels_(kVoxelDimSize, 0) {}

void VoxelArray::delVoxel(int x, int y, int z) {
  voxels_.set(x, y, z, 0);
}

void VoxelArray::setVoxel(int x, int y, int z, RgbTuple color) {
  uint32_t rgba = 255;
  rgba |= std::get<0>(color) << 24;
  rgba |= std::get<1>(color) << 16;
  rgba |= std::get<2>(color) << 8;
  voxels_.set(x, y, z, rgba);
}

size_t VoxelArray::width() const {
  return kVoxelDimSize;
}

size_t VoxelArray::height() const {
  return kVoxelDimSize;
}

size_t VoxelArray::depth() const {
  return kVoxelDimSize;
}

Mesh VoxelArray::toMesh() const {
  enum Dir { X_NEG = 0, X_POS = 1, Y_NEG = 2, Y_POS = 3, Z_NEG = 4, Z_POS = 5 };
  static const std::vector<std::tuple<int, int, int>> kOffsets = {
      {-1, 0, 0},
      {1, 0, 0},
      {0, -1, 0},
      {0, 1, 0},
      {0, 0, -1},
      {0, 0, 1},
  };
  static const std::vector<Eigen::Matrix<float, 3, 6>> kPositions = {
      positionMat<6>({0, 1, 5, 5, 4, 0}),
      positionMat<6>({2, 3, 7, 7, 6, 2}),
      positionMat<6>({0, 3, 2, 2, 1, 0}),
      positionMat<6>({4, 5, 6, 6, 7, 4}),
      positionMat<6>({3, 0, 4, 4, 7, 3}),
      positionMat<6>({1, 2, 6, 6, 5, 1}),
  };
  static const std::vector<Eigen::Matrix<float, 3, 6>> kNormals = {
      normalMat<6>({-1.0f, 0.0f, 0.0f}),
      normalMat<6>({1.0f, 0.0f, 0.0f}),
      normalMat<6>({0.0f, -1.0f, 0.0f}),
      normalMat<6>({0.0f, 1.0f, 0.0f}),
      normalMat<6>({0.0f, 0.0f, -1.0f}),
      normalMat<6>({0.0f, 0.0f, 1.0f}),
  };

  // Generate a vector with every face.
  std::vector<std::tuple<float, float, float, Dir, int32_t>> faces;
  for (int z = 0; z < kVoxelDimSize; z += 1) {
    for (int y = 0; y < kVoxelDimSize; y += 1) {
      for (int x = 0; x < kVoxelDimSize; x += 1) {
        if (auto color = voxels_.get(x, y, z)) {
          for (int i = 0; i < kOffsets.size(); i += 1) {
            int ox = x + std::get<0>(kOffsets.at(i));
            int oy = y + std::get<1>(kOffsets.at(i));
            int oz = z + std::get<2>(kOffsets.at(i));
            if (std::min({ox, oy, oz}) < 0 ||
                std::max({ox, oy, oz}) >= kVoxelDimSize ||
                !voxels_.get(ox, oy, oz)) {
              faces.emplace_back(
                  static_cast<float>(x),
                  static_cast<float>(y),
                  static_cast<float>(z),
                  static_cast<Dir>(i),
                  color);
            }
          }
        }
      }
    }

    Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 6 * faces.size());
    Eigen::Matrix<float, 3, Eigen::Dynamic> normals(3, 6 * faces.size());
    Eigen::Matrix<float, 3, Eigen::Dynamic> colors(3, 6 * faces.size());
    for (int i = 0; i < faces.size(); i += 1) {
      static auto ones_row = Eigen::Matrix<float, 1, 6>::Ones();

      const auto& face = faces.at(i);
      auto fx = std::get<0>(face);
      auto fy = std::get<1>(face);
      auto fz = std::get<2>(face);
      auto dir = static_cast<int>(std::get<3>(face));
      auto color = std::get<4>(face);

      // Set the positions.
      positions.block(0, 6 * i, 3, 6) =
          kPositions.at(dir) + Eigen::Vector3f(fx, fy, fz) * ones_row;

      // Set the normals.
      normals.block(0, 6 * i, 3, 6) = kNormals.at(dir);

      // Set the colors.
      float r = ((color >> 24) & 255) / 255.0f;
      float g = ((color >> 16) & 255) / 255.0f;
      float b = ((color >> 8) & 255) / 255.0f;
      colors.block(0, 6 * i, 3, 6) = Eigen::Vector3f(r, g, b) * ones_row;
    }

    return MeshBuilder()
        .setPositions(std::move(positions))
        .setNormals(std::move(normals))
        .setColors(std::move(colors))
        .build();
  }
}
}  // namespace tequila