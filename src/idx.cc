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

  size_t cur_loc = 0;
  values.reserve(128);

  // The actual parsing is here
  for (auto i = mmap.cbegin(); i != mmap.cend(); ++i) {
    if (*i == '\n' && !in_quote) {
      if (columns == 0) {
        columns = values.size();
      }
      values.push_back(cur_loc + 1);
    }

    else if (*i == delim && !in_quote) {
      values.push_back(cur_loc + 1);
    }

    else if (*i == quote) {
      in_quote = !in_quote;
    }

    ++cur_loc;
  }

  return std::make_tuple(
      std::make_shared<std::vector<size_t> >(values), columns, mmap);
}
