#include "index.h"

#include "simd.h"

#include <fstream>

using namespace vroom;

index::index(
    const char* filename,
    const char* delim,
    const char quote,
    const bool trim_ws,
    const bool escape_double,
    const bool escape_backslash,
    const bool has_header,
    const size_t skip,
    const char comment,
    size_t num_threads,
    bool progress)
    : filename_(filename),
      has_header_(has_header),
      quote_(quote),
      trim_ws_(trim_ws),
      escape_double_(escape_double),
      escape_backslash_(escape_backslash),
      skip_(skip),
      comment_(comment),
      rows_(0),
      columns_(0),
      progress_(progress),
      delim_len_(0) {

  constexpr size_t chunk_size = 1 << 17;
  auto buf = std::vector<char>(chunk_size);

  std::FILE* fp = std::fopen(filename_.c_str(), "rb");

  std::fseek(fp, 0, SEEK_END);
  size_t file_size = std::ftell(fp);
  std::rewind(fp);

  size_t readb = std::fread(buf.data(), 1, chunk_size, fp);

  size_t start = find_first_line(buf, skip_, comment_);

  std::string delim_;

  if (delim == nullptr) {
    delim_ = std::string(1, guess_delim(buf, start));
  } else {
    delim_ = delim;
  }

  delim_len_ = delim_.length();

  size_t first_nl = find_next_newline(buf, start);
  size_t second_nl = find_next_newline(buf, first_nl + 1);
  size_t one_row_size = second_nl - first_nl;
  size_t guessed_rows =
      one_row_size > 0 ? (file_size - first_nl) / one_row_size * 1.1 : 0;

  // Check for windows newlines
  windows_newlines_ = first_nl > 0 && buf[first_nl - 1] == '\r';

  std::unique_ptr<RProgress::RProgress> pb = nullptr;

  if (progress_) {
    auto format = get_pb_format("file", filename);
    auto width = get_pb_width(format);
    pb = std::unique_ptr<RProgress::RProgress>(
        new RProgress::RProgress(format, file_size, width));
    pb->tick(0);
  }

  //
  // We want at least 10 lines per batch, otherwise threads aren't really
  // useful
  // size_t batch_size = file_size / num_threads;
  // size_t line_size = second_nl - first_nl;
  // if (batch_size < line_size * 10) {
  // num_threads = 1;
  //}

  idx_ = std::vector<idx_t>(2); // num_threads + 1);

  // Index the first row
  idx_[0].push_back(start - 1);
  index_region_simd(
      buf.data(), idx_[0], delim_.c_str()[0], quote, start, first_nl + 1, 0);
  columns_ = idx_[0].size() - 1;

  idx_[1].reserve(guessed_rows * columns_);

  size_t total_read = 0;
  while (readb > 0) {
    if (readb < chunk_size) {
      std::memset(buf.data() + readb, '\0', chunk_size - readb);
    }
    index_region_simd(
        buf.data(),
        idx_[1],
        delim_.c_str()[0],
        quote,
        first_nl,
        readb,
        total_read);

    if (progress_) {
      pb->tick(readb);
    }

    total_read += readb;
    readb = std::fread(buf.data(), 1, chunk_size, fp);
  }

  std::fclose(fp);
  if (progress_) {
    pb->update(1);
  }

  size_t total_size = std::accumulate(
      idx_.begin(), idx_.end(), 0, [](size_t sum, const idx_t& v) {
        sum += v.size() - 1;
        return sum;
      });

  rows_ = columns_ > 0 ? total_size / columns_ : 0;

  if (rows_ > 0 && has_header_) {
    --rows_;
  }

#if DEBUG
  std::ofstream log(
      "index.idx",
      std::fstream::out | std::fstream::binary | std::fstream::trunc);
  for (auto& i : idx_) {
    for (auto& v : i) {
      log << v << '\n';
    }
    log << "---\n";
  }
  log.close();
  Rcpp::Rcerr << columns_ << ':' << rows_ << '\n';
#endif

  std::error_code error;
  mmap_ = mio::make_mmap_source(filename_.c_str(), error);
}

void index::trim_quotes(const char*& begin, const char*& end) const {
  if (begin != end && (*begin == quote_)) {
    ++begin;
  }

  if (end != begin && *(end - 1) == quote_) {
    --end;
  }
}

inline bool isspace(const char* c) { return *c == ' ' || *c == '\t'; }

void index::trim_whitespace(const char*& begin, const char*& end) const {
  while (begin != end && isspace(*begin)) {
    ++begin;
  }

  while (end != begin && isspace(*(end - 1))) {
    --end;
  }
}

const string index::get_escaped_string(
    const char* begin, const char* end, bool has_quote) const {
  // If not escaping just return without a copy
  if (!((escape_double_ && has_quote) || escape_backslash_)) {
    return {begin, end};
  }

  std::string out;
  out.reserve(end - begin);

  while (begin < end) {
    if ((escape_double_ && has_quote && *begin == quote_) ||
        (escape_backslash_ && *begin == '\\')) {
      ++begin;
    }

    out.push_back(*begin++);
  }

  return out;
}

std::pair<const char*, const char*>
index::get_cell(size_t i, bool is_first) const {

#if DEBUG
  auto oi = i;
#endif

  for (const auto& idx : idx_) {
    auto sz = idx.size();
    if (i + 1 < sz) {
      size_t start = idx[i] + delim_len_;
      size_t end = idx[i + 1];
      return {mmap_.data() + start, mmap_.data() + end};
    }

    i -= (sz - 1);
#if DEBUG
    Rcpp::Rcerr << "oi: " << oi << " i: " << i << " sz: " << sz << '\n';
#endif
  }

  throw std::out_of_range("blah");
  /* should never get here */
  return {0, 0};
}

const string
index::get_trimmed_val(size_t i, bool is_first, bool is_last) const {

  const char *begin, *end;
  std::tie(begin, end) = get_cell(i, is_first);

  if (is_last && windows_newlines_) {
    end--;
  }

  if (trim_ws_) {
    trim_whitespace(begin, end);
  }

  bool has_quote = false;
  if (quote_ != '\0') {
    has_quote = *begin == quote_;
    if (has_quote) {
      trim_quotes(begin, end);
    }
  }

  return get_escaped_string(begin, end, has_quote);
}

const string index::get(size_t row, size_t col) const {
  auto i = (row + has_header_) * columns_ + col;

  return get_trimmed_val(i, col == 0, col == (columns_ - 1));
}

index::column::iterator::iterator(
    const index& idx, size_t column, size_t start, size_t end)
    : idx_(&idx), column_(column), start_(start + idx_->has_header_) {
  i_ = (start_ * idx_->columns_) + column_;
  is_first_ = column == 0;
  is_last_ = column == (idx_->columns_ - 1);
}

index::column::iterator index::column::iterator::operator++(int) /* postfix */ {
  index::column::iterator copy(*this);
  ++*this;
  return copy;
}
index::column::iterator& index::column::iterator::operator++() /* prefix */ {
  i_ += idx_->columns_;
  return *this;
}

bool index::column::iterator::
operator!=(const index::column::iterator& other) const {
  return i_ != other.i_;
}
bool index::column::iterator::
operator==(const index::column::iterator& other) const {
  return i_ == other.i_;
}

string index::column::iterator::operator*() {
  return idx_->get_trimmed_val(i_, is_first_, is_last_);
}

index::column::iterator& index::column::iterator::operator+=(int n) {
  i_ += idx_->columns_ * n;
  return *this;
}

index::column::iterator index::column::iterator::operator+(int n) {
  index::column::iterator out(*this);
  out += n;
  return out;
}

// Class column
index::column::column(const index& idx, size_t column)
    : idx_(idx), column_(column) {}

index::column::iterator index::column::begin() {
  return index::column::iterator(idx_, column_, 0, idx_.num_rows());
}
index::column::iterator index::column::end() {
  return index::column::iterator(
      idx_, column_, idx_.num_rows(), idx_.num_rows());
}

index::row::iterator::iterator(
    const index& idx, size_t row, size_t start, size_t end)
    : idx_(&idx), row_(row), start_(start) {

  i_ = (row_ + idx_->has_header_) * idx_->columns_ + start_;
}

index::row::iterator index::row::iterator::operator++(int) /* postfix */ {
  index::row::iterator copy(*this);
  ++*this;
  return copy;
}
index::row::iterator& index::row::iterator::operator++() /* prefix */ {
  ++i_;
  return *this;
}

bool index::row::iterator::operator!=(const index::row::iterator& other) const {
  return i_ != other.i_;
}
bool index::row::iterator::operator==(const index::row::iterator& other) const {
  return i_ == other.i_;
}

string index::row::iterator::operator*() {
  return idx_->get_trimmed_val(i_, i_ == 0, i_ == (idx_->columns_ - 1));
}

index::row::iterator& index::row::iterator::operator+=(int n) {
  i_ += n;
  return *this;
}

index::row::iterator index::row::iterator::operator+(int n) {
  index::row::iterator out(*this);
  out += n;
  return out;
}

// Class row
index::row::row(const index& idx, size_t row) : idx_(idx), row_(row) {}

index::row::iterator index::row::begin() {
  return index::row::iterator(idx_, row_, 0, idx_.num_rows());
}
index::row::iterator index::row::end() {
  return index::row::iterator(
      idx_, row_, idx_.num_columns(), idx_.num_columns());
}
