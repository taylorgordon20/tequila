#include "src/common/spatial.hpp"
#include "src/common/images.hpp"

namespace tequila {

void run() {
  auto pixels = loadPngToTensor("images/spatial_test.png");
}

}  // namespace tequila

int main() {
  tequila::run();
  return 0;
}