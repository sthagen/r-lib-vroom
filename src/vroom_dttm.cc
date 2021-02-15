#include "vroom_dttm.h"

double parse_dttm(
    const char* begin,
    const char* end,
    DateTimeParser& parser,
    const std::string& format) {
  parser.setDate(begin, end);
  bool res = (format == "") ? parser.parseISO8601() : parser.parse(format);

  if (res) {
    DateTime dt = parser.makeDateTime();
    if (dt.validDateTime()) {
      return dt.datetime();
    }
  }
  return NA_REAL;
}

cpp11::doubles read_dttm(vroom_vec_info* info) {
  R_xlen_t n = info->column->size();

  cpp11::writable::doubles out(n);
  auto err_msg = info->format.size() == 0
                     ? std::string("date in ISO8601")
                     : std::string("date like ") + info->format;

  try {
    parallel_for(
        n,
        [&](size_t start, size_t end, size_t) {
          R_xlen_t i = start;
          DateTimeParser parser(info->locale.get());
          auto col = info->column->slice(start, end);
          for (auto b = col->begin(), e = col->end(); b != e; ++b) {
            out[i++] = parse_value<double>(
                b,
                col,
                [&](const char* begin, const char* end) -> double {
                  return parse_dttm(begin, end, parser, info->format);
                },
                info->errors,
                err_msg.c_str(),
                *info->na);
          }
        },
        info->num_threads,
        true);
  } catch (const std::runtime_error& e) {
    Rf_errorcall(R_NilValue, "%s", e.what());
  }

  info->errors->warn_for_errors();

  out.attr("class") = {"POSIXct", "POSIXt"};
  out.attr("tzone") = info->locale->tz_;

  return out;
}

#ifdef HAS_ALTREP

R_altrep_class_t vroom_dttm::class_t;

void init_vroom_dttm(DllInfo* dll) { vroom_dttm::Init(dll); }

#else
void init_vroom_dttm(DllInfo* dll) {}
#endif

[[cpp11::register]] cpp11::writable::doubles utctime_(
    cpp11::integers year,
    cpp11::integers month,
    cpp11::integers day,
    cpp11::integers hour,
    cpp11::integers min,
    cpp11::integers sec,
    cpp11::doubles psec) {
  int n = year.size();
  if (month.size() != n || day.size() != n || hour.size() != n ||
      min.size() != n || sec.size() != n || psec.size() != n) {
    cpp11::stop("All inputs must be same length");
  }

  cpp11::writable::doubles out(n);

  for (int i = 0; i < n; ++i) {
    DateTime dt(
        year[i],
        month[i] - 1,
        day[i] - 1,
        hour[i],
        min[i],
        sec[i],
        psec[i],
        "UTC");
    out[i] = dt.datetime();
  }

  out.attr("class") = {"POSIXct", "POSIXt"};
  out.attr("tzone") = "UTC";

  return out;
}
