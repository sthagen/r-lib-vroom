#include "vroom_chr.h"

SEXP check_na(SEXP na, SEXP val) {
  for (R_xlen_t i = 0; i < Rf_xlength(na); ++i) {
    SEXP v = STRING_ELT(na, i);
    // We can just compare the addresses directly because they should now
    // both be in the global string cache.
    if (v == val) {
      return NA_STRING;
    }
  }
  return val;
}

cpp11::strings read_chr(vroom_vec_info* info) {

  R_xlen_t n = info->column->size();

  cpp11::writable::strings out(n);

  SEXP nas = *info->na;

  cpp11::unwind_protect([&] {
    auto i = 0;
    for (const auto& str : *info->column) {
      auto val = info->locale->encoder_.makeSEXP(str.begin(), str.end(), false);

      SET_STRING_ELT(out, i++, check_na(nas, val));
    }
  });

  return out;
}

#ifdef HAS_ALTREP

R_altrep_class_t vroom_chr::class_t;

void init_vroom_chr(DllInfo* dll) { vroom_chr::Init(dll); }

#else
void init_vroom_chr(DllInfo* dll) {}
#endif
