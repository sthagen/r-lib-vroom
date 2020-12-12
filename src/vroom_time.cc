#include "vroom_time.h"

double parse_time(
    const string& str, DateTimeParser& parser, const std::string& format) {
  parser.setDate(str.begin(), str.end());
  bool res = (format == "") ? parser.parseLocaleTime() : parser.parse(format);

  if (res) {
    DateTime dt = parser.makeTime();
    if (dt.validTime()) {
      return dt.time();
    }
  }
  return NA_REAL;
}

cpp11::doubles read_time(vroom_vec_info* info) {
  R_xlen_t n = info->column->size();

  cpp11::writable::doubles out(n);

  parallel_for(
      n,
      [&](size_t start, size_t end, size_t) {
        R_xlen_t i = start;
        DateTimeParser parser(info->locale.get());
        auto col = info->column->slice(start, end);
        for (const auto& str : *col) {
          out[i++] = parse_time(str, parser, info->format);
        }
      },
      info->num_threads,
      true);

  out.attr("class") = {"hms", "difftime"};
  out.attr("units") = "secs";

  return out;
}

#ifdef HAS_ALTREP

R_altrep_class_t vroom_time::class_t;

void init_vroom_time(DllInfo* dll) { vroom_time::Init(dll); }

#else
void init_vroom_time(DllInfo* dll) {}
#endif
