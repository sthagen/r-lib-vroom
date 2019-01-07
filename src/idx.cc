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

  // The actual parsing is here
  auto i = strpbrk(mmap.cbegin(), query);
  while (i != nullptr) {
    if (*i == '\n' && !in_quote) {
      if (columns == 0) {
        columns = values.size();
      }
      values.push_back(i - start + 1);
    }

    else if (*i == delim && !in_quote) {
      values.push_back(i - start + 1);
    }

    else if (*i == quote) {
      in_quote = !in_quote;
    }

    i = strpbrk(i + 1, query);
  }

  return std::make_tuple(
      std::make_shared<std::vector<size_t> >(values), columns, mmap);
}
