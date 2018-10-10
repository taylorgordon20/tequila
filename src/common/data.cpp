#include "src/common/data.hpp"

#include <zstd.h>

#include <sstream>
#include <string>

#include "src/common/errors.hpp"
#include "src/common/files.hpp"
#include "src/common/timers.hpp"

namespace tequila {

namespace {
auto tablePath(const std::string& table_name) {
  auto data_dir = resolvePathOrThrow("data");
  return format("./%1%/%2%.db", data_dir, table_name);
}

auto compress(const std::string& src) {
  std::vector<char> dst(ZSTD_compressBound(src.size()), '\0');
  auto csize = ZSTD_compress(dst.data(), dst.size(), src.data(), src.size(), 7);
  ENFORCE(
      !ZSTD_isError(csize),
      format("ZSTD_compress error: %1%", ZSTD_getErrorName(csize)));
  dst.resize(csize);
  return dst;
}

auto decompress(const std::vector<char>& src) {
  auto dsize = ZSTD_getDecompressedSize(src.data(), src.size());
  ENFORCE(dsize, "Error inferring decompressed data size.");
  std::string dst(dsize, '\0');
  auto status = ZSTD_decompress(dst.data(), dst.size(), src.data(), src.size());
  ENFORCE(
      !ZSTD_isError(status),
      format(
          "ZSTD_decompress error: %1% (src.size()=%2%, dsize=%3%)",
          ZSTD_getErrorName(status),
          src.size(),
          dsize));
  return dst;
}
}  // namespace

Table::Table(const std::string& name) : db_(tablePath(name)) {
  db_ << "CREATE TABLE IF NOT EXISTS blobs (key TEXT PRIMARY KEY, blob BLOB);";
}

bool Table::has(const std::string& key) {
  int count = 0;
  db_ << "SELECT COUNT(*) FROM blobs WHERE key = ?" << key >> count;
  return count > 0;
}

void Table::del(const std::string& key) {
  db_ << "DELETE FROM blobs WHERE key = ?" << key;
}

void Table::set(const std::string& key, const std::string& data) {
  db_ << "REPLACE INTO blobs VALUES (?, ?);" << key << compress(data);
}

std::string Table::get(const std::string& key) {
  std::vector<char> data;
  db_ << "SELECT blob FROM blobs WHERE key = ?;" << key >> data;
  return decompress(data);
}

}  // namespace tequila