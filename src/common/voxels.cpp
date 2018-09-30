#include "src/common/voxels.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/meshes.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

constexpr auto kVoxelArraySize = 64;

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

template <int cols>
auto texCoordMat() {
  Eigen::Matrix<float, 2, cols> mat;
  mat.row(0) << 0, 1, 1, 1, 0, 0;
  mat.row(1) << 0, 0, 1, 1, 1, 0;
  return mat;
}
}  // anonymous namespace

VoxelArray::VoxelArray()
    : voxels_(kVoxelArraySize, 0), transform_(glm::mat4(1.0f)) {}

bool VoxelArray::has(int x, int y, int z) const {
  return voxels_.get(x, y, z);
}

void VoxelArray::del(int x, int y, int z) {
  voxels_.set(x, y, z, 0);
}

void VoxelArray::set(int x, int y, int z, uint32_t value) {
  voxels_.set(x, y, z, value);
}

uint32_t VoxelArray::get(int x, int y, int z) const {
  return voxels_.get(x, y, z);
}

void VoxelArray::translate(float x, float y, float z) {
  transform_ = glm::translate(transform_, glm::vec3(x, y, z));
}

void VoxelArray::rotate(float x, float y, float z, float angle) {
  transform_ = glm::rotate(transform_, angle, glm::vec3(x, y, z));
}

void VoxelArray::scale(float x, float y, float z) {
  transform_ = glm::scale(transform_, glm::vec3(x, y, z));
}

const glm::mat4& VoxelArray::transform() const {
  return transform_;
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
  static const std::vector<Eigen::Matrix<float, 3, 6>> kTangents = {
      normalMat<6>({0.0f, 0.0f, 1.0f}),
      normalMat<6>({0.0f, 0.0f, -1.0f}),
      normalMat<6>({0.0f, 0.0f, -1.0f}),
      normalMat<6>({0.0f, 0.0f, 1.0f}),
      normalMat<6>({-1.0f, 0.0f, 0.0f}),
      normalMat<6>({1.0f, 0.0f, 0.0f}),
  };
  static const std::vector<Eigen::Matrix<float, 3, 6>> kCotangents = {
      normalMat<6>({0.0f, 1.0f, 0.0f}),
      normalMat<6>({0.0f, 1.0f, 0.0f}),
      normalMat<6>({1.0f, 0.0f, 0.0f}),
      normalMat<6>({1.0f, 0.0f, 0.0f}),
      normalMat<6>({0.0f, 1.0f, 0.0f}),
      normalMat<6>({0.0f, 1.0f, 0.0f}),
  };
  static const Eigen::Matrix<float, 2, 6> kTexCoords = texCoordMat<6>();

  // Generate a vector with every face.
  std::vector<std::tuple<float, float, float, Dir, int32_t>> faces;
  for (int z = 0; z < voxels_.depth(); z += 1) {
    for (int y = 0; y < voxels_.height(); y += 1) {
      for (int x = 0; x < voxels_.width(); x += 1) {
        if (auto color = voxels_.get(x, y, z)) {
          for (int i = 0; i < kOffsets.size(); i += 1) {
            int ox = x + std::get<0>(kOffsets.at(i));
            int oy = y + std::get<1>(kOffsets.at(i));
            int oz = z + std::get<2>(kOffsets.at(i));
            bool a = ox < 0 || ox >= voxels_.width();
            bool b = oy < 0 || oy >= voxels_.height();
            bool c = oz < 0 || oz >= voxels_.depth();
            if (a || b || c || !voxels_.get(ox, oy, oz)) {
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
  }

  Eigen::Matrix<float, 3, Eigen::Dynamic> positions(3, 6 * faces.size());
  Eigen::Matrix<float, 3, Eigen::Dynamic> normals(3, 6 * faces.size());
  Eigen::Matrix<float, 3, Eigen::Dynamic> tangents(3, 6 * faces.size());
  Eigen::Matrix<float, 3, Eigen::Dynamic> colors(3, 6 * faces.size());
  Eigen::Matrix<float, 2, Eigen::Dynamic> tex_coords(2, 6 * faces.size());
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

    // Set the tangents.
    tangents.block(0, 6 * i, 3, 6) = kTangents.at(dir);

    // Set the texture coordinates.
    tex_coords.block(0, 6 * i, 2, 6) = kTexCoords;
    tex_coords.row(0).segment(6 * i, 6) +=
        Eigen::Vector3f(fx, fy, fz).transpose() * kTangents.at(dir);
    tex_coords.row(1).segment(6 * i, 6) +=
        Eigen::Vector3f(fx, fy, fz).transpose() * kCotangents.at(dir);

    // Set the colors.
    float r = ((color >> 24) & 255) / 255.0f;
    float g = ((color >> 16) & 255) / 255.0f;
    float b = ((color >> 8) & 255) / 255.0f;
    colors.block(0, 6 * i, 3, 6) = Eigen::Vector3f(r, g, b) * ones_row;
  }

  return MeshBuilder()
      .setPositions(std::move(positions))
      .setNormals(std::move(normals))
      .setTangents(std::move(tangents))
      .setColors(std::move(colors))
      .setTexCoords(std::move(tex_coords))
      .setTransform(transform_)
      .build();
}

}  // namespace tequila