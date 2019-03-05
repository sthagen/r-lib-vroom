#pragma once

#include <x86intrin.h>

#define really_inline inline __attribute__((always_inline, unused))
#define never_inline inline __attribute__((noinline, unused))

#define UNUSED __attribute__((unused))
#define WARN_UNUSED __attribute__((warn_unused_result))

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

// this is more efficient apparently than the scalar code above (fewer
// instructions)
static inline bool is_made_of_eight_digits_fast(const char* chars) {
  __m64 val;
  memcpy(&val, chars, 8);
  __m64 base = _mm_sub_pi8(val, _mm_set1_pi8('0'));
  __m64 basecmp = _mm_subs_pu8(base, _mm_set1_pi8(9));
  return _mm_cvtm64_si64(basecmp) == 0;
}

static inline uint32_t parse_eight_digits_unrolled(const char* chars) {
  // this actually computes *16* values so we are being wasteful.
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  const __m128i mul_1_10000 =
      _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1);
  const __m128i input = _mm_sub_epi8(
      _mm_loadu_si128(reinterpret_cast<const __m128i*>(chars)), ascii0);
  const __m128i t1 = _mm_maddubs_epi16(input, mul_1_10);
  const __m128i t2 = _mm_madd_epi16(t1, mul_1_100);
  const __m128i t3 = _mm_packus_epi32(t2, t2);
  const __m128i t4 = _mm_madd_epi16(t3, mul_1_10000);
  return _mm_cvtsi128_si32(
      t4); // only captures the sum of the first 8 digits, drop the rest
}

static inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// parse the number at buf + offset
// define JSON_TEST_NUMBERS for unit testing
static really_inline double parse_number(const char* p) {
  bool negative = false;
  if (*p == '-') {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
      return NA_REAL;
    }
  }
  const char* const startdigits = p;

  int64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    // if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
    //#ifdef JSON_TEST_NUMBERS // for unit testing
    // foundInvalidNumber(buf + offset);
    //#endif
    // return false;
    //}
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
      return NA_REAL;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      i = 10 * i + digit; // might overflow
      ++p;
    }
  }

  int64_t exponent = 0;

  if ('.' == *p) {
    ++p;
    const char* const firstafterperiod = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit;
    } else {
      return false;
    }
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      // exponent -= 8;
    }
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = firstafterperiod - p;
  }
  int digitcount = p - startdigits - 1;

  int64_t expnumber = 0; // exponential part
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool negexp = false;
    if ('-' == *p) {
      negexp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
      return NA_REAL;
    }
    unsigned char digit = *p - '0';
    expnumber = digit;
    p++;
    while (is_integer(*p)) {
      digit = *p - '0';
      expnumber = 10 * expnumber + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      expnumber = 10 * expnumber + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      expnumber = 10 * expnumber + digit;
      ++p;
    }
    if (is_integer(*p)) {
      // we refuse to parse this
      return NA_REAL;
    }
    exponent += (negexp ? -expnumber : expnumber);
  }
  i = negative ? -i : i;
  if ((exponent != 0) || (expnumber != 0)) {
    if (unlikely(digitcount >= 19)) { // this is uncommon!!!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      return NA_REAL;
    }
    ///////////
    // We want 0.1e1 to be a float.
    //////////
    if (i == 0) {
      return 0.0;
    } else {
      if ((exponent > 308) || (exponent < -308)) {
        // we refuse to parse this
        return NA_REAL;
      }
      double d = i;
      d *= power_of_ten[308 + exponent];
      // d = negative ? -d : d;
      return d;
    }
  } else {
    if (unlikely(digitcount >= 18)) { // this is uncommon!!!
      return NA_REAL;
    }
    return i;
  }
  return NA_REAL;
}

/*
    An STL iterator-based string to floating point number conversion.
    This function was adapted from the C standard library of RetroBSD,
    which is based on Berkeley UNIX.
    This function and this function only is BSD license.

    https://retrobsd.googlecode.com/svn/stable/src/libc/stdlib/strtod.c
   */
template <class Iterator> double bsd_strtod(Iterator begin, Iterator end) {
  if (begin == end) {
    return NA_REAL;
  }
  int sign = 0, expSign = 0, i;
  double fraction, dblExp;
  Iterator p = begin;
  char c;

  /* Exponent read from "EX" field. */
  int exp = 0;

  /* Exponent that derives from the fractional part.  Under normal
   * circumstances, it is the negative of the number of digits in F.
   * However, if I is very long, the last digits of I get dropped
   * (otherwise a long I with a large negative exponent could cause an
   * unnecessary overflow on I alone).  In this case, fracExp is
   * incremented one for each dropped digit. */
  int fracExp = 0;

  /* Number of digits in mantissa. */
  int mantSize;

  /* Number of mantissa digits BEFORE decimal point. */
  int decPt;

  /* Temporarily holds location of exponent in str. */
  Iterator pExp;

  /* Largest possible base 10 exponent.
   * Any exponent larger than this will already
   * produce underflow or overflow, so there's
   * no need to worry about additional digits. */
  static int maxExponent = 307;

  /* Table giving binary powers of 10.
   * Entry is 10^2^i.  Used to convert decimal
   * exponents into floating-point numbers. */
  static double powersOf10[] = {
      1e1, 1e2, 1e4, 1e8, 1e16, 1e32, // 1e64, 1e128, 1e256,
  };
#if 0
        static double powersOf2[] = {
                2, 4, 16, 256, 65536, 4.294967296e9, 1.8446744073709551616e19,
                //3.4028236692093846346e38, 1.1579208923731619542e77, 1.3407807929942597099e154,
        };
        static double powersOf8[] = {
                8, 64, 4096, 2.81474976710656e14, 7.9228162514264337593e28,
                //6.2771017353866807638e57, 3.9402006196394479212e115, 1.5525180923007089351e231,
        };
        static double powersOf16[] = {
                16, 256, 65536, 1.8446744073709551616e19,
                //3.4028236692093846346e38, 1.1579208923731619542e77, 1.3407807929942597099e154,
        };
#endif

  /*
   * Strip off leading blanks and check for a sign.
   */
  p = begin;
  while (p != end && (*p == ' ' || *p == '\t'))
    ++p;
  if (p != end && *p == '-') {
    sign = 1;
    ++p;
  } else if (p != end && *p == '+')
    ++p;

  /* If we don't have a digit or decimal point something is wrong, so return an
   * NA */
  if (!(isdigit(*p) || *p == '.')) {
    return NA_REAL;
  }

  /*
   * Count the number of digits in the mantissa (including the decimal
   * point), and also locate the decimal point.
   */
  decPt = -1;
  for (mantSize = 0; p != end; ++mantSize) {
    c = *p;
    if (!isdigit(c)) {
      if (c != '.' || decPt >= 0)
        break;
      decPt = mantSize;
    }
    ++p;
  }

  /*
   * Now suck up the digits in the mantissa.  Use two integers to
   * collect 9 digits each (this is faster than using floating-point).
   * If the mantissa has more than 18 digits, ignore the extras, since
   * they can't affect the value anyway.
   */
  pExp = p;
  p -= mantSize;
  if (decPt < 0)
    decPt = mantSize;
  else
    --mantSize; /* One of the digits was the point. */

  if (mantSize > 2 * 9)
    mantSize = 2 * 9;
  fracExp = decPt - mantSize;
  if (mantSize == 0) {
    fraction = 0.0;
    p = begin;
    goto done;
  } else {
    int frac1, frac2;

    for (frac1 = 0; mantSize > 9 && p != end; --mantSize) {
      c = *p++;
      if (c == '.')
        c = *p++;
      frac1 = frac1 * 10 + (c - '0');
    }
    for (frac2 = 0; mantSize > 0 && p != end; --mantSize) {
      c = *p++;
      if (c == '.')
        c = *p++;
      frac2 = frac2 * 10 + (c - '0');
    }
    fraction = (double)1000000000 * frac1 + frac2;
  }

  /*
   * Skim off the exponent.
   */
  p = pExp;
  if (p != end &&
      (*p == 'E' || *p == 'e' || *p == 'S' || *p == 's' || *p == 'F' ||
       *p == 'f' || *p == 'D' || *p == 'd' || *p == 'L' || *p == 'l')) {
    ++p;
    if (p != end && *p == '-') {
      expSign = 1;
      ++p;
    } else if (p != end && *p == '+')
      ++p;
    while (p != end && isdigit(*p))
      exp = exp * 10 + (*p++ - '0');
  }
  if (expSign)
    exp = fracExp - exp;
  else
    exp = fracExp + exp;

  /*
   * Generate a floating-point number that represents the exponent.
   * Do this by processing the exponent one bit at a time to combine
   * many powers of 2 of 10. Then combine the exponent with the
   * fraction.
   */
  if (exp < 0) {
    expSign = 1;
    exp = -exp;
  } else
    expSign = 0;
  if (exp > maxExponent)
    exp = maxExponent;
  dblExp = 1.0;
  for (i = 0; exp; exp >>= 1, ++i)
    if (exp & 01)
      dblExp *= powersOf10[i];
  if (expSign)
    fraction /= dblExp;
  else
    fraction *= dblExp;

done:
  return sign ? -fraction : fraction;
}
