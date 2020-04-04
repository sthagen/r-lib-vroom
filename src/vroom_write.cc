#include "grisu3.h"
#include <Rcpp.h>
#include <array>
#include <future>
#include <iterator>

#include "RProgress.h"
#include "connection.h"
#include "r_utils.h"

#include "unicode_fopen.h"

typedef enum {
  quote_needed = 1,
  quote_all = 2,
  escape_double = 4,
  escape_backslash = 8,
  bom = 16
} vroom_write_opt_t;

size_t get_buffer_size(
    const Rcpp::List& input,
    const std::vector<SEXPTYPE>& types,
    size_t start,
    size_t end) {

  // First need to determine how big the buffer(s) should be
  // - For characters we need the total nchar() + 2 (for quotes if needed)
  //   (they are converted to UTF-8 in R)
  // - For factors we need max(nchar(levels)) (but currently we just convert to
  //   character in R)
  // - For decimal numbers we need 24
  //   source: https://stackoverflow.com/a/52045523/2055486
  // - For 32 bit integers we need 11 (10 for digits plus the sign)
  // - For logical we need 5 (FALSE)
  //
  // - Currently we convert dates, times and datetimes to character before
  //   output. If we wanted to do it in C it would be
  //   - For dates we need 10 (2019-04-12)
  //   - For times we need 8 (01:00:00)
  //   - For datetimes we need 20 (2019-04-12T20:46:31Z)

  size_t buf_size = 0;

  size_t num_rows = end - start;

  for (int i = 0; i < input.length(); ++i) {
    switch (types[i]) {
    case STRSXP: {
      for (size_t j = start; j < end; ++j) {
        auto sz = Rf_xlength(STRING_ELT(input[i], j));
        buf_size += sz + 2;
      }
      break;
    }
    case LGLSXP:
      buf_size += 5 * num_rows;
      break;
    case REALSXP:
      buf_size += 24 * num_rows;
      break;
    case INTSXP:
      buf_size += 11 * num_rows;
      break;
    }
  }

  // Add size of delimiters + newline
  buf_size += input.length() * num_rows;

  return buf_size;
}

bool needs_quote(const char* str, const char delim, const char* na_str) {
  if (strncmp(str, na_str, 2) == 0) {
    return true;
  }

  for (const char* cur = str; *cur != '\0'; ++cur) {
    if (*cur == '\n' || *cur == '\r' || *cur == '"' || *cur == delim) {
      return true;
    }
  }

  return false;
}

// adapted from https://stackoverflow.com/a/28110728/2055486
template <size_t N>
void append_literal(std::vector<char>& buf, const char (&str)[N]) {
  std::copy(std::begin(str), std::end(str) - 1, std::back_inserter(buf));
}

inline bool is_utf8(cetype_t ce) {
  switch (ce) {
  case CE_ANY:
  case CE_BYTES:
  case CE_UTF8:
    return true;
  default:
    return false;
  }
}

void str_to_buf(
    SEXP str,
    std::vector<char>& buf,
    const char delim,
    const char* na_str,
    size_t na_len,
    size_t options) {

  if (str == NA_STRING) {
    std::copy(na_str, na_str + na_len, std::back_inserter(buf));
    return;
  }

  const char* str_p;
  if (is_utf8(Rf_getCharCE(str))) {
    str_p = CHAR(str);
  } else {
    str_p = Rf_translateCharUTF8(str);
  }

  auto len = Rf_xlength(str);
  bool should_quote =
      options & quote_all ||
      (options & quote_needed && needs_quote(str_p, delim, na_str));
  if (should_quote) {
    buf.push_back('"');
  }

  auto end = str_p + len;
  bool should_escape = options & (escape_double | escape_backslash);
  auto escape =
      options & escape_double ? '"' : options & escape_backslash ? '\\' : '\0';

  buf.reserve(buf.size() + len);
  while (str_p < end) {
    if (should_escape && *str_p == '"') {
      buf.push_back(escape);
    }
    buf.push_back(*str_p++);
  }

  if (should_quote) {
    buf.push_back('"');
  }
  return;
}

std::vector<char> fill_buf(
    const Rcpp::List& input,
    const char delim,
    const char* na_str,
    size_t options,
    const std::vector<SEXPTYPE>& types,
    const std::vector<void*>& ptrs,
    size_t begin,
    size_t end) {

  auto buf = std::vector<char>();

  auto na_len = strlen(na_str);

  for (size_t row = begin; row < end; ++row) {
    for (int col = 0; col < input.length(); ++col) {
      switch (types[col]) {
      case STRSXP: {
        auto str = STRING_ELT(input[col], row);
        str_to_buf(str, buf, delim, na_str, na_len, options);
        break;
      }
      case LGLSXP: {
        int value = static_cast<int*>(ptrs[col])[row];
        switch (value) {
        case TRUE:
          append_literal(buf, "TRUE");
          break;
        case FALSE:
          append_literal(buf, "FALSE");
          break;
        default:
          std::copy(na_str, na_str + na_len, std::back_inserter(buf));
          break;
        }
        break;
      }
      case REALSXP: {
        auto value = static_cast<double*>(ptrs[col])[row];
        if (!R_FINITE(value)) {
          if (ISNA(value)) {
            std::copy(na_str, na_str + na_len, std::back_inserter(buf));
          } else if (ISNAN(value)) {
            append_literal(buf, "NaN");
          } else if (value > 0) {
            append_literal(buf, "Inf");
          } else {
            append_literal(buf, "-Inf");
          }
        } else {
          char temp_buf[33];
          int len = dtoa_grisu3(static_cast<double*>(ptrs[col])[row], temp_buf);
          std::copy(temp_buf, temp_buf + len, std::back_inserter(buf));
        }
        break;
      }
      case INTSXP: {
        auto value = static_cast<int*>(ptrs[col])[row];
        if (value == NA_INTEGER) {
          std::copy(na_str, na_str + na_len, std::back_inserter(buf));
        } else {
          // TODO: use something like https://github.com/jeaiii/itoa for
          // faster integer writing
          char temp_buf[12];
          auto len = sprintf(temp_buf, "%i", value);
          std::copy(temp_buf, temp_buf + len, std::back_inserter(buf));
        }
        break;
      }
      }
      buf.push_back(delim);
    }
    buf[buf.size() - 1] = '\n';
  }

  return buf;
}

void write_buf(const std::vector<char>& buf, std::FILE* out) {
  std::fwrite(buf.data(), sizeof buf[0], buf.size(), out);
}

#ifdef VROOM_USE_CONNECTIONS_API
void write_buf_con(
    const std::vector<char>& buf, Rconnection con, bool is_stdout) {
  if (is_stdout) {
    std::string out;
    std::copy(buf.begin(), buf.end(), std::back_inserter(out));
    Rcpp::Rcout << out;
  } else {
    R_WriteConnection(con, (void*)buf.data(), sizeof buf[0] * buf.size());
  }
}
#else
void write_buf_con(const std::vector<char>& buf, SEXP con, bool is_stdout) {
  if (is_stdout) {
    std::string out;
    std::copy(buf.begin(), buf.end(), std::back_inserter(out));
    Rcpp::Rcout << out;
  } else {
    R_WriteConnection(con, (void*)buf.data(), sizeof buf[0] * buf.size());
  }
}
#endif

std::vector<SEXPTYPE> get_types(const Rcpp::List& input) {
  std::vector<SEXPTYPE> out;
  for (int col = 0; col < input.length(); ++col) {
    out.push_back(TYPEOF(input[col]));
  }
  return out;
}

std::vector<void*> get_ptrs(const Rcpp::List& input) {
  std::vector<void*> out;
  for (int col = 0; col < input.length(); ++col) {
    switch (TYPEOF(input[col])) {
    case REALSXP:
      out.push_back(REAL(input[col]));
      break;
    case INTSXP:
      out.push_back(INTEGER(input[col]));
      break;
    case LGLSXP:
      out.push_back(LOGICAL(input[col]));
      break;
    default:
      out.push_back(nullptr);
    }
  }
  return out;
}

std::vector<char>
get_header(const Rcpp::List& input, const char delim, size_t options) {
  Rcpp::CharacterVector names =
      Rcpp::as<Rcpp::CharacterVector>(input.attr("names"));
  std::vector<char> out;
  for (R_xlen_t i = 0; i < names.size(); ++i) {
    auto str = STRING_ELT(names, i);

    str_to_buf(str, out, delim, "", 0, options);
    out.push_back(delim);
  }
  out[out.size() - 1] = '\n';
  return out;
}

// [[Rcpp::export]]
void vroom_write_(
    Rcpp::List input,
    std::string filename,
    const char delim,
    const char* na_str,
    bool col_names,
    bool append,
    size_t options,
    size_t num_threads,
    bool progress,
    size_t buf_lines) {

  size_t begin = 0;
  size_t num_rows = Rf_xlength(input[0]);

  char mode[3] = "wb";
  if (append) {
    strcpy(mode, "ab");
  }

  std::FILE* out = unicode_fopen(filename.c_str(), mode);
  if (!out) {
    std::string msg("Cannot open file for writing:\n* ");
    msg += '\'' + filename + '\'';
    throw Rcpp::exception(msg.c_str(), false);
  }

  std::array<std::vector<std::future<std::vector<char> > >, 2> futures;
  futures[0].resize(num_threads);
  futures[1].resize(num_threads);

  std::future<size_t> write_fut;

  int idx = 0;

  auto types = get_types(input);
  auto ptrs = get_ptrs(input);

  if (!append && options & bom) {
    std::vector<char> bom{'\xEF', '\xBB', '\xBF'};
    write_buf(bom, out);
  }

  if (col_names) {
    auto header = get_header(input, delim, options);
    write_buf(header, out);
  }

  std::unique_ptr<RProgress::RProgress> pb = nullptr;
  if (progress) {
    pb = std::unique_ptr<RProgress::RProgress>(
        new RProgress::RProgress(vroom::get_pb_format("write"), 1e12));
  }

  while (begin < num_rows) {
    size_t t = 0;
    while (t < num_threads && begin < num_rows) {
      auto num_lines = std::min(buf_lines, num_rows - begin);
      auto end = begin + num_lines;
      futures[idx][t++] = std::async(
          fill_buf, input, delim, na_str, options, types, ptrs, begin, end);
      begin += num_lines;
    }

    if (write_fut.valid()) {
      auto sz = write_fut.get();
      if (progress) {
        pb->tick(sz);
      }
    }

    write_fut = std::async([&, idx, t] {
      size_t sz = 0;
      for (size_t i = 0; i < t; ++i) {
        auto buf = futures[idx][i].get();
        write_buf(buf, out);
        sz += buf.size();
      }
      return sz;
    });

    idx = (idx + 1) % 2;
  }

  // Wait for the last writing to finish
  if (write_fut.valid()) {
    write_fut.get();
    if (progress) {
      pb->update(1);
    }
  }

  // Close the file
  std::fclose(out);
}

// TODO: Think about refactoring this so it and vroom_write_ can share some
// code
// [[Rcpp::export]]
void vroom_write_connection_(
    Rcpp::List input,
    Rcpp::RObject con,
    const char delim,
    const char* na_str,
    bool col_names,
    size_t options,
    size_t num_threads,
    bool progress,
    size_t buf_lines,
    bool is_stdout) {

  size_t begin = 0;
  size_t num_rows = Rf_xlength(input[0]);

  auto con_ = R_GetConnection(con);

  bool should_open = !is_open(con);
  if (should_open) {
    Rcpp::as<Rcpp::Function>(Rcpp::Environment::base_env()["open"])(con, "wb");
  }

  bool should_close = should_open;

  std::array<std::vector<std::future<std::vector<char> > >, 2> futures;
  futures[0].resize(num_threads);
  futures[1].resize(num_threads);

  std::future<size_t> write_fut;

  int idx = 0;

  auto types = get_types(input);
  auto ptrs = get_ptrs(input);

  if (col_names) {
    auto header = get_header(input, delim, options);
    write_buf_con(header, con_, is_stdout);
  }

  std::unique_ptr<RProgress::RProgress> pb = nullptr;
  if (progress) {
    pb = std::unique_ptr<RProgress::RProgress>(
        new RProgress::RProgress(vroom::get_pb_format("write"), 1e12));
  }

  while (begin < num_rows) {
    size_t t = 0;
    while (t < num_threads && begin < num_rows) {
      auto num_lines = std::min(buf_lines, num_rows - begin);
      auto end = begin + num_lines;
      futures[idx][t++] = std::async(
          fill_buf, input, delim, na_str, options, types, ptrs, begin, end);
      begin += num_lines;
    }

    // if (write_fut.valid()) {
    // auto sz = write_fut.get();
    // Rcpp::checkUserInterrupt();
    //}

    // write_fut = std::async([&, idx, t] {
    // size_t sz = 0;
    for (size_t i = 0; i < t; ++i) {
      auto buf = futures[idx][i].get();
      write_buf_con(buf, con_, is_stdout);
      auto sz = buf.size();
      if (progress) {
        pb->tick(sz);
      }
      // sz += buf.size();
    }
    // return sz;
    //});

    idx = (idx + 1) % 2;
  }

  // Wait for the last writing to finish
  // if (write_fut.valid()) {
  // write_fut.get();
  if (progress) {
    pb->update(1);
  }
  //}

  // Close the connection
  if (should_close) {
    Rcpp::as<Rcpp::Function>(Rcpp::Environment::base_env()["close"])(con);
  }
}

// [[Rcpp::export]]
Rcpp::CharacterVector vroom_format_(
    Rcpp::List input,
    const char delim,
    const char* na_str,
    bool col_names,
    size_t options) {

  size_t num_rows = Rf_xlength(input[0]);
  Rcpp::CharacterVector out(1);

  auto types = get_types(input);
  auto ptrs = get_ptrs(input);

  std::vector<char> data;

  if (options & bom) {
    std::vector<char> bom{'\xEF', '\xBB', '\xBF'};
    std::copy(bom.begin(), bom.end(), std::back_inserter(data));
  }

  if (col_names) {
    data = get_header(input, delim, options);
  }

  auto buf = fill_buf(input, delim, na_str, options, types, ptrs, 0, num_rows);
  std::copy(buf.begin(), buf.end(), std::back_inserter(data));

  out[0] = Rf_mkCharLenCE(data.data(), data.size(), CE_UTF8);

  return out;
}
