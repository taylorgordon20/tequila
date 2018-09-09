#include <unordered_map>

#include "src/common/errors.hpp"
#include "src/common/meshes.hpp"
#include "src/common/spatial.hpp"

namespace tequila {

class VoxelArray {
 public:
  using RgbTuple = std::tuple<uint8_t, uint8_t, uint8_t>;
  VoxelArray();
  void del(int x, int y, int z);
  void set(int x, int y, int z, RgbTuple color);
  RgbTuple get(int x, int y, int z) const;
  Mesh toMesh() const;

  template <typename Archive>
  void serialize(Archive& archive) {
    archive(voxels_);
  }

 private:
  CubeStore<128, uint32_t> voxels_;
};

}  // namespace tequila