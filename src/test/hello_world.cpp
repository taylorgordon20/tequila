#include <zlib.h>
#include <Eigen/Dense>
#include <iostream>
#include <string>
#include <vector>

std::string compress(const std::string& data) {
  size_t size = data.size();
  size_t pad_size = sizeof(size);
  uLongf dst_size = compressBound(size);
  const Bytef* bytes = reinterpret_cast<const Bytef*>(data.c_str());
  std::vector<Bytef> out(pad_size + dst_size);
  memcpy(out.data(), &size, pad_size);
  if (compress2(out.data() + pad_size, &dst_size, bytes, size, -1) != Z_OK) {
    throw std::runtime_error("Zlib error compressing");
  }
  out.resize(pad_size + dst_size);  // Zlib overwrites to actual output size.
  return std::string(out.begin(), out.end());
}

std::string decompress(const std::string& encoding) {
  unsigned long out_size;
  size_t size = encoding.size();
  size_t pad_size = sizeof(size_t);
  memcpy(&out_size, encoding.c_str(), pad_size);

  const Bytef* bytes = reinterpret_cast<const Bytef*>(encoding.c_str());
  std::vector<Bytef> out(out_size);
  if (uncompress(out.data(), &out_size, bytes + pad_size, size) != Z_OK) {
    throw std::runtime_error("Zlib error decompressing");
  }
  assert(out_size == out.size());

  return std::string(out.begin(), out.end());
}
int main() {
  // Print some stuff and return.
  std::cout << "Hello world! Let's compress some data!" << std::endl;

  std::string data = "big funny data";
  std::cout << "Data is: " << data << std::endl;

  auto compressed = compress(data);
  std::cout << "Compressed data is: " << compressed << std::endl;

  auto decompressed = decompress(compressed);
  std::cout << "Decompressed data is: " << decompressed << std::endl;

  return 0;
}