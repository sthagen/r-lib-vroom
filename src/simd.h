#include "mio/mmap.hpp"
#include "portability.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define SIMDJSON_PADDING sizeof(__m256i)

#define really_inline inline __attribute__((always_inline, unused))
#define never_inline inline __attribute__((noinline, unused))
#define WARN_UNUSED __attribute__((warn_unused_result))

char* allocate_padded_buffer(size_t length) {
  // we could do a simple malloc
  // return (char *) malloc(length + SIMDJSON_PADDING);
  // However, we might as well align to cache lines...
  char* padded_buffer;
  size_t totalpaddedlength = length + SIMDJSON_PADDING;
  if (posix_memalign(
          reinterpret_cast<void**>(&padded_buffer), 64, totalpaddedlength) !=
      0) {
    return nullptr;
  }
  return padded_buffer;
}

//
// a straightforward comparison of a mask against input. 5 uops; would be
// cheaper in AVX512.
really_inline uint64_t
cmp_mask_against_input(__m256i input_lo, __m256i input_hi, __m256i mask) {
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(input_lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(input_hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}

inline void dump256(__m256i d, std::string msg) {
  for (uint32_t i = 0; i < 32; i++) {
    std::cout << std::setw(3) << (int)*(((uint8_t*)(&d)) + i);
    if (!((i + 1) % 8))
      std::cout << "|";
    else if (!((i + 1) % 4))
      std::cout << ":";
    else
      std::cout << " ";
  }
  std::cout << " " << msg << "\n";
}

// dump bits low to high
void dumpbits(uint64_t v, std::string msg) {
  for (uint32_t i = 0; i < 64; i++) {
    std::cout << (((v >> (uint64_t)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

void dumpbits32(uint32_t v, std::string msg) {
  for (uint32_t i = 0; i < 32; i++) {
    std::cout << (((v >> (uint32_t)i) & 0x1ULL) ? "1" : "_");
  }
  std::cout << " " << msg << "\n";
}

using idx_t = std::vector<size_t>;

template <typename T>
WARN_UNUSED never_inline bool index_region_simd(
    const T& buf,
    idx_t& destination,
    char delim,
    char quote,
    size_t start,
    size_t end,
    uint64_t file_offset) {

  // Useful constant masks
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;

  // for now, just work in 64-byte chunks
  // we have padded the input out to 64 byte multiple with the remainder being
  // zeros

  // persistent state across loop
  uint64_t prev_iter_ends_odd_backslash =
      0ULL;                               // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_inside_quote = 0ULL; // either all zeros or all ones

  // effectively the very first char is considered to follow "whitespace" for
  // the purposes of psuedo-structural character detection
  uint64_t prev_iter_ends_pseudo_pred = 1ULL;
  size_t idx = start;
  uint64_t structurals = 0;
  for (; idx < end; idx += 64) {
    __m256i input_lo =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buf + idx + 0));
    __m256i input_hi =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buf + idx + 32));

    // Find backslashes
    uint64_t bs_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
    // flip lowest if we have an odd-length run at the end of the prior
    // iteration
    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
    uint64_t even_starts = start_edges & even_start_mask;
    uint64_t odd_starts = start_edges & ~even_start_mask;
    uint64_t even_carries = bs_bits + even_starts;

    uint64_t odd_carries;
    // must record the carry-out of our odd-carries out of bit 63; this
    // indicates whether the sense of any edge going to the next iteration
    // should be flipped
    bool iter_ends_odd_backslash =
        add_overflow(bs_bits, odd_starts, &odd_carries);

    odd_carries |=
        prev_iter_ends_odd_backslash; // push in bit zero as a potential end
                                      // if we had an odd-numbered run at the
                                      // end of the previous iteration
    prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
    uint64_t even_carry_ends = even_carries & ~bs_bits;
    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;

    // Find quotes
    uint64_t quote_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8(quote));
    quote_bits = quote_bits & ~odd_ends;

    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
    quote_mask ^= prev_iter_inside_quote;

    prev_iter_inside_quote = static_cast<uint64_t>(
        static_cast<int64_t>(quote_mask) >>
        63); // right shift of a signed value expected to be well-defined and
             // standard compliant as of C++20, John Regher from Utah U. says
             // this is fine code
             //
    // Find delimiters
    uint64_t delim_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8(delim));
    // dumpbits(delim_bits, "delim_bits");
    delim_bits = delim_bits & ~odd_ends;

    // Find newlines
    uint64_t nl_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\n'));
    // dumpbits(nl_bits, "nl_bits");
    nl_bits = nl_bits & ~odd_ends;

    uint64_t structurals = 0;
    structurals |= delim_bits;
    structurals |= nl_bits;

    // Mask off anything inside quotes
    structurals &= ~quote_mask;

    size_t cutoff = end + file_offset;
    while (structurals != 0u) {
      size_t loc = idx + trailingzeroes(structurals) + file_offset;
      if (loc > cutoff) {
        break;
      }
      destination.push_back(loc);
      structurals = structurals & (structurals - 1);
    }
  }
  return true;
}
