#include "vroom_int.h"
#include "parallel.h"

// A version of strtoi that doesn't need null terminated strings, to avoid
// needing to copy the data
int strtoi(const char* begin, const char* end) {
  double val = 0;
  bool is_neg = false;

  if (begin == end) {
    return NA_INTEGER;
  }

  if (begin != end && *begin == '-') {
    is_neg = true;
    ++begin;
  }

  while (begin != end && isdigit(*begin)) {
    val = val * 10 + ((*begin++) - '0');
  }

  // If there is more than digits, return NA
  if (begin != end) {
    return NA_INTEGER;
  }

  if (val > INT_MAX) {
    return NA_INTEGER;
  }

  return is_neg ? -val : val;
}

// Normal reading of integer vectors
cpp11::integers read_int(vroom_vec_info* info) {

  R_xlen_t n = info->column->size();

  cpp11::writable::integers out(n);

  parallel_for(
      n,
      [&](size_t start, size_t end, size_t) {
        R_xlen_t i = start;
        auto col = info->column->slice(start, end);
        for (auto b = col->begin(), e = col->end(); b != e; ++b) {
          out[i++] = parse_value<int>(
              b, col, strtoi, info->errors, "an integer", *info->na);
        }
      },
      info->num_threads);

  info->errors->warn_for_errors();

  return out;
}

#ifdef HAS_ALTREP

R_altrep_class_t vroom_int::class_t;

void init_vroom_int(DllInfo* dll) { vroom_int::Init(dll); }

#else
void init_vroom_int(DllInfo* dll) {}
#endif
