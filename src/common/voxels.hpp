#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/meshes.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

class VoxelArray {
 public:
  using RgbTuple = std::tuple<uint8_t, uint8_t, uint8_t>;
  VoxelArray();

  void delVoxel(int x, int y, int z);
  void setVoxel(int x, int y, int z, RgbTuple color);
  size_t width() const;
  size_t height() const;
  size_t depth() const;

  Mesh toMesh() const;

 private:
  CubeStore<uint32_t> voxels_;
};

}  // namespace tequila