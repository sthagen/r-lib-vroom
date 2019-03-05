#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <x86intrin.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fcntl.h>

#define SIMDJSON_PADDING  sizeof(__m256i)

#define really_inline inline __attribute__((always_inline, unused))
#define never_inline inline __attribute__((noinline, unused))
#define WARN_UNUSED __attribute__((warn_unused_result))

/* result might be undefined when input_num is zero */
static inline int hamming(uint64_t input_num) {
	return _popcnt64(input_num);
}

/* result might be undefined when input_num is zero */
static inline int trailingzeroes(uint64_t input_num) {
#ifdef __BMI__
	return _tzcnt_u64(input_num);
#else
#warning "BMI is missing?"
	return __builtin_ctzll(input_num);
#endif
}

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

static inline void aligned_free(void* memblock) {
  if (memblock == nullptr) {
    return;
  }
  free(memblock);
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
		std::cout << std::setw(3) << (int)*(((uint8_t *)(&d)) + i);
        if (!((i+1)%8))
            std::cout << "|";
        else if (!((i+1)%4))
            std::cout << ":";
        else
            std::cout << " ";
	}
    std::cout << " " << msg << "\n";
}

 // dump bits low to high
void dumpbits(uint64_t v, std::string msg) {
	for (uint32_t i = 0; i < 64; i++) {
        std::cout << (((v>>(uint64_t)i) & 0x1ULL) ? "1" : "_");
    }
    std::cout << " " << msg << "\n";
}

 void dumpbits32(uint32_t v, std::string msg) {
	for (uint32_t i = 0; i < 32; i++) {
        std::cout << (((v>>(uint32_t)i) & 0x1ULL) ? "1" : "_");
    }
    std::cout << " " << msg << "\n";
}

using idx_t = std::vector<size_t>;

template <typename T>
WARN_UNUSED
never_inline bool index_region(const T& buf, idx_t& destination, size_t len, char delim, uint64_t file_offset) {

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
  size_t lenminus64 = len < 64 ? 0 : len - 64;
  size_t idx = 0;
  uint64_t structurals = 0;
  for (; idx < len; idx += 64) {
    __m256i input_lo =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buf + idx + 0));
    __m256i input_hi =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buf + idx + 32));

    // Find delimiters
    uint64_t delim_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8(delim));
    //dumpbits(delim_bits, "delim_bits");

    // Find newlines
    uint64_t nl_bits =
        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\n'));
    //dumpbits(nl_bits, "nl_bits");

    uint64_t structurals = 0;
    structurals |= delim_bits;
    structurals |= nl_bits;

    while (structurals != 0u) {
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                          
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);                   
      structurals = structurals & (structurals - 1);
      destination.push_back(static_cast<uint64_t>(idx) - 64 + trailingzeroes(structurals) + file_offset);
      structurals = structurals & (structurals - 1);
    }
  }
  return true;
}
//    ////////////////////////////////////////////////////////////////////////////////////////////
//    //     Step 1: detect odd sequences of backslashes
//    ////////////////////////////////////////////////////////////////////////////////////////////
//
//    uint64_t bs_bits =
//        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('\\'));
//    uint64_t start_edges = bs_bits & ~(bs_bits << 1);
//    // flip lowest if we have an odd-length run at the end of the prior
//    // iteration
//    uint64_t even_start_mask = even_bits ^ prev_iter_ends_odd_backslash;
//    uint64_t even_starts = start_edges & even_start_mask;
//    uint64_t odd_starts = start_edges & ~even_start_mask;
//    uint64_t even_carries = bs_bits + even_starts;
//
//    uint64_t odd_carries;
//    // must record the carry-out of our odd-carries out of bit 63; this
//    // indicates whether the sense of any edge going to the next iteration
//    // should be flipped
//    bool iter_ends_odd_backslash =
//        add_overflow(bs_bits, odd_starts, &odd_carries);
//
//    odd_carries |=
//        prev_iter_ends_odd_backslash; // push in bit zero as a potential end
//                                      // if we had an odd-numbered run at the
//                                      // end of the previous iteration
//    prev_iter_ends_odd_backslash = iter_ends_odd_backslash ? 0x1ULL : 0x0ULL;
//    uint64_t even_carry_ends = even_carries & ~bs_bits;
//    uint64_t odd_carry_ends = odd_carries & ~bs_bits;
//    uint64_t even_start_odd_end = even_carry_ends & odd_bits;
//    uint64_t odd_start_even_end = odd_carry_ends & even_bits;
//    uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
//
//    ////////////////////////////////////////////////////////////////////////////////////////////
//    //     Step 2: detect insides of quote pairs
//    ////////////////////////////////////////////////////////////////////////////////////////////
//
//    uint64_t quote_bits =
//        cmp_mask_against_input(input_lo, input_hi, _mm256_set1_epi8('"'));
//    quote_bits = quote_bits & ~odd_ends;
//    uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
//        _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
//
//    uint32_t cnt = hamming(structurals);
//    uint32_t next_base = base + cnt;
//    while (structurals != 0u) {
//      base_ptr[base + 0] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 1] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 2] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 3] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 4] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 5] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 6] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base_ptr[base + 7] =
//          static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//      structurals = structurals & (structurals - 1);
//      base += 8;
//    }
//    base = next_base;
//
//    quote_mask ^= prev_iter_inside_quote;
//    prev_iter_inside_quote = static_cast<uint64_t>(
//        static_cast<int64_t>(quote_mask) >>
//        63); // right shift of a signed value expected to be well-defined and
//             // standard compliant as of C++20, John Regher from Utah U. says
//             // this is fine code
//
//    // How do we build up a user traversable data structure
//    // first, do a 'shufti' to detect structural JSON characters
//    // they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
//    // these go into the first 3 buckets of the comparison (1/2/4)
//
//    // we are also interested in the four whitespace characters
//    // space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
//    // these go into the next 2 buckets of the comparison (8/16)
//    const __m256i low_nibble_mask = _mm256_setr_epi8(
//        //  0                           9  a   b  c  d
//        16,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        8,
//        12,
//        1,
//        2,
//        9,
//        0,
//        0,
//        16,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        0,
//        8,
//        12,
//        1,
//        2,
//        9,
//        0,
//        0);
//    const __m256i high_nibble_mask = _mm256_setr_epi8(
//        //  0     2   3     5     7
//        8,
//        0,
//        18,
//        4,
//        0,
//        1,
//        0,
//        1,
//        0,
//        0,
//        0,
//        3,
//        2,
//        1,
//        0,
//        0,
//        8,
//        0,
//        18,
//        4,
//        0,
//        1,
//        0,
//        1,
//        0,
//        0,
//        0,
//        3,
//        2,
//        1,
//        0,
//        0);
//
//    __m256i structural_shufti_mask = _mm256_set1_epi8(0x7);
//    __m256i whitespace_shufti_mask = _mm256_set1_epi8(0x18);
//
//    __m256i v_lo = _mm256_and_si256(
//        _mm256_shuffle_epi8(low_nibble_mask, input_lo),
//        _mm256_shuffle_epi8(
//            high_nibble_mask,
//            _mm256_and_si256(
//                _mm256_srli_epi32(input_lo, 4), _mm256_set1_epi8(0x7f))));
//
//    __m256i v_hi = _mm256_and_si256(
//        _mm256_shuffle_epi8(low_nibble_mask, input_hi),
//        _mm256_shuffle_epi8(
//            high_nibble_mask,
//            _mm256_and_si256(
//                _mm256_srli_epi32(input_hi, 4), _mm256_set1_epi8(0x7f))));
//    __m256i tmp_lo = _mm256_cmpeq_epi8(
//        _mm256_and_si256(v_lo, structural_shufti_mask), _mm256_set1_epi8(0));
//    __m256i tmp_hi = _mm256_cmpeq_epi8(
//        _mm256_and_si256(v_hi, structural_shufti_mask), _mm256_set1_epi8(0));
//
//    uint64_t structural_res_0 =
//        static_cast<uint32_t>(_mm256_movemask_epi8(tmp_lo));
//    uint64_t structural_res_1 = _mm256_movemask_epi8(tmp_hi);
//    structurals = ~(structural_res_0 | (structural_res_1 << 32));
//
//    // this additional mask and transfer is non-trivially expensive,
//    // unfortunately
//    __m256i tmp_ws_lo = _mm256_cmpeq_epi8(
//        _mm256_and_si256(v_lo, whitespace_shufti_mask), _mm256_set1_epi8(0));
//    __m256i tmp_ws_hi = _mm256_cmpeq_epi8(
//        _mm256_and_si256(v_hi, whitespace_shufti_mask), _mm256_set1_epi8(0));
//
//    uint64_t ws_res_0 =
//    static_cast<uint32_t>(_mm256_movemask_epi8(tmp_ws_lo)); uint64_t ws_res_1
//    = _mm256_movemask_epi8(tmp_ws_hi); uint64_t whitespace = ~(ws_res_0 |
//    (ws_res_1 << 32));
//    // mask off anything inside quotes
//    structurals &= ~quote_mask;
//
//    // add the real quote bits back into our bitmask as well, so we can
//    // quickly traverse the strings we've spent all this trouble gathering
//    structurals |= quote_bits;
//
//    // Now, establish "pseudo-structural characters". These are non-whitespace
//    // characters that are (a) outside quotes and (b) have a predecessor
//    that's
//    // either whitespace or a structural character. This means that subsequent
//    // passes will get a chance to encounter the first character of every
//    string
//    // of non-whitespace and, if we're parsing an atom like true/false/null or
//    a
//    // number we can stop at the first whitespace or structural character
//    // following it.
//
//    // a qualified predecessor is something that can happen 1 position before
//    an
//    // psuedo-structural character
//    uint64_t pseudo_pred = structurals | whitespace;
//    uint64_t shifted_pseudo_pred =
//        (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
//    prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
//    uint64_t pseudo_structurals =
//        shifted_pseudo_pred & (~whitespace) & (~quote_mask);
//    structurals |= pseudo_structurals;
//
//    // now, we've used our close quotes all we need to. So let's switch them
//    off
//    // they will be off in the quote mask and on in quote bits.
//    structurals &= ~(quote_bits & ~quote_mask);
//
//    //*(uint64_t *)(pj.structurals + idx / 8) = structurals;
//  }
//
//  uint32_t cnt = hamming(structurals);
//  uint32_t next_base = base + cnt;
//  while (structurals != 0u) {
//    base_ptr[base + 0] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 1] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 2] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 3] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 4] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 5] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 6] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base_ptr[base + 7] =
//        static_cast<uint32_t>(idx) - 64 + trailingzeroes(structurals);
//    structurals = structurals & (structurals - 1);
//    base += 8;
//  }
//  base = next_base;
//
//  pj.n_structural_indexes = base;
//  // a valid JSON file cannot have zero structural indexes - we should have
//  // found something
//  if (pj.n_structural_indexes == 0u) {
//    return false;
//  }
//  if (base_ptr[pj.n_structural_indexes - 1] > len) {
//    fprintf(stderr, "Internal bug\n");
//    return false;
//  }
//  if (len != base_ptr[pj.n_structural_indexes - 1]) {
//    // the string might not be NULL terminated, but we add a virtual NULL
//    ending
//    // character.
//    base_ptr[pj.n_structural_indexes++] = len;
//  }
//  base_ptr[pj.n_structural_indexes] =
//      0; // make it safe to dereference one beyond this array
//
//  return true;
//#endif
//}

int main(int argc, char** argv) {
  std::FILE* fp = std::fopen(argv[1], "rb");
  if (fp == nullptr) {
    return 1;
  }

  constexpr size_t buf_size = 1 << 17;

  //std::fseek(fp, 0, SEEK_END);
  //size_t len = std::ftell(fp);
  char* buf = allocate_padded_buffer(buf_size);
  if (buf == nullptr) {
    std::fclose(fp);
    throw std::runtime_error("could not allocate memory");
  }
  //std::rewind(fp);
  idx_t idx;
  idx.reserve(1135001640);
  size_t offset = 0;
  size_t readb = std::fread(buf, 1, buf_size, fp);
  while(readb > 0) {
    if (readb < buf_size) {
      std::memset(buf + readb, '\0', buf_size - readb);
    }
    bool res = index_region(buf, idx, readb, ',', offset);
    offset += readb;
    readb = std::fread(buf, 1, buf_size, fp);
  }

  std::cout << "Index: " << idx.size() << '\n';

  std::fclose(fp);
  aligned_free(buf);

  return 0;
}
