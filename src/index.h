#ifndef READIDX_IDX_HEADER
#define READIDX_IDX_HEADER

// clang-format off
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wsign-compare"
#include <mio/shared_mmap.hpp>
# pragma clang diagnostic pop
// clang-format on

#include <array>

#include "multi_progress.h"

#include <Rcpp.h>
#include "utils.h"

namespace vroom {

struct cell {
  const char* begin;
  const char* end;
};

// A custom string wrapper that avoids constructing a string object unless
// needed because of escapes.
class string {
public:
  string(std::string str) : str_(str) {
    begin_ = str_.c_str();
    end_ = begin_ + str_.length();
  }
  string(const char* begin, const char* end) : begin_(begin), end_(end) {}

  const char* begin() const { return begin_; }

  const char* end() const { return end_; }

  size_t length() const { return end_ - begin_; }

  size_t size() const { return end_ - begin_; }

  bool operator==(const string& other) const {
    return length() == other.length() &&
           strncmp(begin_, other.begin_, length()) == 0;
  }

  bool operator==(const std::string& other) const {
    return length() == other.length() &&
           strncmp(begin_, other.data(), length()) == 0;
  }

private:
  const char* begin_;
  const char* end_;
  std::string str_;
};

class index {

public:
  index(
      const char* filename,
      const char* delim,
      const char quote,
      const bool trim_ws,
      const bool escape_double,
      const bool escape_backslash,
      const bool has_header,
      const size_t skip,
      const char comment,
      const size_t num_threads,
      const bool progress);

  class column {
    const index& idx_;
    size_t column_;

  public:
    column(const index& idx, size_t column);

    class iterator {
      size_t i_;
      const index* idx_;
      size_t column_;
      size_t start_;
      bool is_first_;
      bool is_last_;

    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = string;
      using pointer = string*;
      using reference = string&;

      iterator(const index& idx, size_t column, size_t start, size_t end);
      iterator operator++(int); /* postfix */
      iterator& operator++();   /* prefix */
      bool operator!=(const iterator& other) const;
      bool operator==(const iterator& other) const;

      string operator*();
      iterator& operator+=(int n);
      iterator operator+(int n);
    };
    iterator begin();
    iterator end();
  };

  class row {
    const index& idx_;
    size_t row_;

  public:
    row(const index& idx, size_t row);

    class iterator {
      size_t i_;
      const index* idx_;
      size_t row_;
      size_t start_;

    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = string;
      using pointer = string*;
      using reference = string&;

      iterator(const index& idx, size_t row, size_t start, size_t end);
      iterator operator++(int); /* postfix */
      iterator& operator++();   /* prefix */
      bool operator!=(const iterator& other) const;
      bool operator==(const iterator& other) const;

      string operator*();
      iterator& operator+=(int n);
      iterator operator+(int n);
    };
    iterator begin();
    iterator end();
  };

  index() : rows_(0), columns_(0){};

  const string get(size_t row, size_t col) const;

  size_t num_columns() const { return columns_; }

  size_t num_rows() const { return rows_; }

  std::string filename() const { return filename_; }

  column get_column(size_t col) const {
    return vroom::index::column(*this, col);
  }

  row get_row(size_t row) const { return vroom::index::row(*this, row); }

  row get_header() const { return vroom::index::row(*this, -1); }

public:
  using idx_t = std::vector<size_t>;
  std::string filename_;
  mio::mmap_source mmap_;
  std::vector<idx_t> idx_;
  bool has_header_;
  char quote_;
  bool trim_ws_;
  bool escape_double_;
  bool escape_backslash_;
  bool windows_newlines_;
  size_t skip_;
  char comment_;
  size_t rows_;
  size_t columns_;
  bool progress_;
  size_t delim_len_;
  std::locale loc_;

  void skip_lines();

  void trim_quotes(const char*& begin, const char*& end) const;
  void trim_whitespace(const char*& begin, const char*& end) const;
  const string
  get_escaped_string(const char* begin, const char* end, bool has_quote) const;

  const string get_trimmed_val(size_t i, bool is_first, bool is_last) const;

  std::pair<const char*, const char*> get_cell(size_t i, bool is_first) const;

  /*
   * @param source the source to index
   * @param destination the index to push to
   * @param delim the delimiter to use
   * @param quote the quoting character
   * @param start the start of the region to index
   * @param end the end of the region to index
   * @param offset an offset to add to the destination (this is needed when
   * @param pb the progress bar to use
   * @param update_size how often to update the progress bar
   * reading blocks from a connection).
   */
  template <typename T, typename P>
  void index_region(
      const T& source,
      idx_t& destination,
      const char* delim,
      const char quote,
      const size_t start,
      const size_t end,
      const size_t file_offset,
      P& pb,
      const size_t update_size = -1) {

    // If there are no quotes quote will be '\0', so will just work
    std::array<char, 5> query = {delim[0], '\n', '\\', quote, '\0'};

    auto last_tick = start;
    auto num_ticks = 0;

    bool in_quote = false;

    auto buf = source.data();

    // The actual parsing is here
    size_t pos = start;
    while (pos < end) {
      size_t buf_offset = strcspn(buf + pos, query.data());
      pos = pos + buf_offset;
      auto c = buf[pos];

      if (!in_quote && strncmp(delim, buf + pos, delim_len_) == 0) {
        destination.push_back(pos + file_offset);
      }

      else if (c == '\n') { // no embedded quotes allowed
        destination.push_back(pos + file_offset);
        if (progress_ && pb) {
          auto tick_size = pos - last_tick;
          if (tick_size > update_size) {
            pb->tick(pos - last_tick);
            last_tick = pos;
            ++num_ticks;
          }
        }
      }

      else if (c == quote) {
        in_quote = !in_quote;
      }

      else if (escape_backslash_ && c == '\\') {
        ++pos;
      }

      ++pos;
    }

    if (progress_ && pb) {
      pb->tick(end - last_tick);
    }
    // Rcpp::Rcerr << num_ticks << '\n';
  }
};

} // namespace vroom

// Specialization for our custom strings, needed so we can use them in
// unordered_maps
// [1]: https://stackoverflow.com/a/17017281/2055486
// [2]: https://stackoverflow.com/a/34597485/2055486
namespace std {

template <> struct hash<vroom::string> {
  std::size_t operator()(const vroom::string& k) const {
    const char* begin = k.begin();
    const char* end = k.end();

    size_t result = 0;
    const size_t prime = 31;
    while (begin != end) {
      result = *begin++ + (result * prime);
    }
    return result;
  }
};

} // namespace std

#endif /* READIDX_IDX_HEADER */
