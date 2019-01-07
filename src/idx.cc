#include "idx.h"

std::tuple<
    std::shared_ptr<std::vector<size_t> >,
    size_t,
    mio::shared_mmap_source>
create_index(const char* filename, char delim, char quote, int num_threads) {
  size_t columns = 0;

  std::error_code error;
  mio::shared_mmap_source mmap = mio::make_mmap_source(filename, error);

  if (error) {
    throw Rcpp::exception(error.message().c_str(), false);
  }

  std::vector<size_t> values;

  values.push_back(0);

  bool in_quote = false;

  values.reserve(128);

  char query[8] = {delim, '\n', quote};

  auto start = mmap.cbegin();
  size_t last = 0;
  size_t file_size = mmap.cend() - mmap.cbegin();

  // The actual parsing is here
  auto i = strcspn(mmap.cbegin(), query) + last;
  while (i < file_size) {
    auto c = mmap[i];
    if (c == '\n' && !in_quote) {
      if (columns == 0) {
        columns = values.size();
      }
      values.push_back(i + 1);
    }

    else if (c == delim && !in_quote) {
      values.push_back(i + 1);
    }

    else if (c == quote) {
      in_quote = !in_quote;
    }

    last = i;
    i = strcspn(start + i + 1, query) + last + 1;
  }

  return std::make_tuple(
      std::make_shared<std::vector<size_t> >(values), columns, mmap);
}
