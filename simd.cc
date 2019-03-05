#include "src/simd.h"

int main(int argc, char** argv) {

  std::FILE* fp = std::fopen(argv[1], "rb");
  if (fp == nullptr) {
    return 1;
  }

  constexpr size_t buf_size = 1 << 16;

  //std::fseek(fp, 0, SEEK_END);
  //size_t len = std::ftell(fp);
  char* buf = allocate_padded_buffer(buf_size);
  if (buf == nullptr) {
    std::fclose(fp);
    throw std::runtime_error("could not allocate memory");
  }
  //std::rewind(fp);
  idx_t idx;
  idx.reserve(500000050);
  size_t offset = 0;
  size_t readb = std::fread(buf, 1, buf_size, fp);
  size_t count =0;
  while(readb > 0) {
    if (readb < buf_size) {
      std::memset(buf + readb, '\0', buf_size - readb);
    }
    count = index_region_simd(buf, idx, ',', '"', 0, readb, offset);
    offset += readb;
    readb = std::fread(buf, 1, buf_size, fp);
  }

  std::cout << "Index: " << idx.size() << '\n';

  std::fclose(fp);
  aligned_free(buf);

  return 0;
}
