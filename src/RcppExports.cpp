// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// read_tsv_
SEXP read_tsv_(const std::string& filename);
RcppExport SEXP _readidx_read_tsv_(SEXP filenameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::string& >::type filename(filenameSEXP);
    rcpp_result_gen = Rcpp::wrap(read_tsv_(filename));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_readidx_read_tsv_", (DL_FUNC) &_readidx_read_tsv_, 1},
    {NULL, NULL, 0}
};

void init_readidx_string(DllInfo* dll);
RcppExport void R_init_readidx(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    init_readidx_string(dll);
}
