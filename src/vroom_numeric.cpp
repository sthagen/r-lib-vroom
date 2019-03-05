#include "Rcpp.h"
#include "parse_dbl.h"
#include "utils.h"
#include <array>

#include <mio/shared_mmap.hpp>

using namespace std;
using namespace Rcpp;

// [[Rcpp::export]]
Rcpp::List vroom_numeric_(
    std::string filename,
    const char* delim,
    size_t skip,
    int num_columns,
    int num_rows) {

  // If there are no quotes quote will be '\0', so will just work
  std::array<char, 3> query = {delim[0], '\n', '\0'};

  std::error_code error;
  auto source = mio::make_mmap_source(filename, error);

  if (error) {
    // We cannot actually portably compare error messages due to a bug in
    // libstdc++ (https://stackoverflow.com/a/54316671/2055486), so just print
    // the message on stderr return
    Rcpp::Rcerr << "mmaping error: " << error.message() << '\n';
    return List();
  }

  auto last_tick = 0;
  auto num_ticks = 0;

  auto buf = source.data();

  size_t start = find_first_line(source, skip, '\0');

  Rcpp::List out(num_columns);
  for (int i = 0; i < num_columns; ++i) {
    out[i] = Rcpp::NumericVector(num_rows);
  }

  // The actual parsing is here
  size_t pos = start;
  size_t end = source.end() - source.begin();
  size_t col = 0;
  size_t row = 0;
  size_t delim_len = strlen(delim);

  while (pos < end) {
    size_t buf_offset = strcspn(buf + pos, query.data());
    pos = pos + buf_offset;
    auto c = buf[pos];

    if (strncmp(delim, buf + pos, delim_len) == 0) {
      auto blah = parse_number(buf + pos + delim_len);
      REAL(VECTOR_ELT(out, col++))[row] = blah;
    } else if (c == '\n') { // no embedded quotes allowed
      auto blah = parse_number(buf + pos + 1);
      REAL(VECTOR_ELT(out, col))[row++] = blah;
      col = 0;
    }

    ++pos;
  }

  out.attr("class") =
      Rcpp::CharacterVector::create("tbl", "tbl_df", "data.frame");
  out.attr("row.names") = Rcpp::IntegerVector::create(NA_INTEGER, -num_rows);
  return out;
}
