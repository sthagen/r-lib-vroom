#ifndef FASTREAD_DATE_TIME_PARSER_H_
#define FASTREAD_DATE_TIME_PARSER_H_

#include "DateTime.h"
#include "LocaleInfo.h"
#include <Rcpp.h>
#include <algorithm>
#include <ctime>

// Parsing ---------------------------------------------------------------------

template <typename Iterator, typename Attr>
inline bool parseInt(Iterator& first, Iterator& last, Attr& res) {

  char buf[64];

  std::copy(first, last, buf);
  buf[last - first] = '\0';

  long lres;
  char* endp;

  errno = 0;
  lres = strtol(buf, &endp, 10);
  if (*endp != '\0')
    lres = NA_INTEGER;
  /* next can happen on a 64-bit platform */
  if (res > INT_MAX || res < INT_MIN)
    lres = NA_INTEGER;
  if (errno == ERANGE)
    lres = NA_INTEGER;

  res = static_cast<int>(lres);

  first += last - first;
  return res != NA_INTEGER;
}

template <typename Iterator, typename Attr>
inline bool parseDouble(
    const char decimalMark, Iterator& first, Iterator& last, Attr& res) {

  char buf[65];

  // It can't be a double if it is over 64 characters long.
  bool too_long = last - first > 64;
  if (too_long) {
    res = NA_REAL;
    return false;
  }

  std::copy(first, last, buf);
  buf[last - first] = '\0';

  char* endp;

  errno = 0;
  res = strtod(buf, &endp);
  if (errno > 0) {
    res = NA_REAL;
  } else {
    first += endp - buf;
  }

  return !ISNA(res);
}

class DateTimeParser {
  int year_, mon_, day_, hour_, min_, sec_;
  double psec_;
  int amPm_;
  bool compactDate_; // used for guessing

  int tzOffsetHours_, tzOffsetMinutes_;
  std::string tz_;

  LocaleInfo* pLocale_;
  std::string tzDefault_;

  const char* dateItr_;
  const char* dateEnd_;

public:
  DateTimeParser(LocaleInfo* pLocale)
      : pLocale_(pLocale),
        tzDefault_(pLocale->tz_),
        dateItr_(NULL),
        dateEnd_(NULL) {
    reset();
  }

  // Parse ISO8601 date time. In benchmarks this only seems ~30% faster than
  // parsing with a format string so it doesn't seem necessary to add individual
  // parsers for other common formats.
  bool parseISO8601(bool partial = true) {
    // Date: YYYY-MM-DD, YYYYMMDD
    if (!consumeInteger(4, &year_))
      return false;
    if (consumeThisChar('-'))
      compactDate_ = false;
    if (!consumeInteger1(2, &mon_))
      return false;
    if (!compactDate_ && !consumeThisChar('-'))
      return false;
    if (!consumeInteger1(2, &day_))
      return false;

    if (isComplete())
      return true;

    // Spec requires T, but common to use space instead
    char next;
    if (!consumeChar(&next))
      return false;
    if (next != 'T' && next != ' ')
      return false;

    // hh:mm:ss.sss, hh:mm:ss, hh:mm, hh
    // hhmmss.sss, hhmmss, hhmm
    if (!consumeInteger(2, &hour_))
      return false;
    consumeThisChar(':');
    consumeInteger(2, &min_);
    consumeThisChar(':');
    consumeSeconds(&sec_, &psec_);

    if (isComplete())
      return true;

    // Has a timezone
    tz_ = "UTC";
    if (!consumeTzOffset(&tzOffsetHours_, &tzOffsetMinutes_))
      return false;

    return isComplete();
  }

  bool parseLocaleTime() { return parse(pLocale_->timeFormat_); }

  bool parseLocaleDate() { return parse(pLocale_->dateFormat_); }

  // A flexible time parser for the most common formats
  bool parseTime() {
    if (!consumeInteger(2, &hour_, false))
      return false;
    if (!consumeThisChar(':'))
      return false;
    if (!consumeInteger(2, &min_))
      return false;
    consumeThisChar(':');
    consumeSeconds(&sec_, NULL);

    consumeWhiteSpace();
    consumeString(pLocale_->amPm_, &amPm_);
    consumeWhiteSpace();

    return isComplete();
  }

  bool parseDate() {
    // Date: YYYY-MM-DD, YYYY/MM/DD
    if (!consumeInteger(4, &year_))
      return false;
    if (!consumeThisChar('-') && !consumeThisChar('/'))
      return false;
    if (!consumeInteger1(2, &mon_))
      return false;
    if (!consumeThisChar('-') && !consumeThisChar('/'))
      return false;
    if (!consumeInteger1(2, &day_))
      return false;

    return isComplete();
  }

  bool isComplete() { return dateItr_ == dateEnd_; }

  void setDate(const char* start, const char* end) {
    reset();
    dateItr_ = start;
    dateEnd_ = end;
  }

  bool parse(const std::string& format) {
    consumeWhiteSpace(); // always consume leading whitespace

    std::string::const_iterator formatItr, formatEnd = format.end();
    for (formatItr = format.begin(); formatItr != formatEnd; ++formatItr) {
      // Whitespace in format matches 0 or more whitespace in date
      if (std::isspace(*formatItr)) {
        consumeWhiteSpace();
        continue;
      }

      // Any other characters must much exactly.
      if (*formatItr != '%') {
        if (!consumeThisChar(*formatItr))
          return false;
        continue;
      }

      if (formatItr + 1 == formatEnd)
        Rcpp::stop("Invalid format: trailing %");
      formatItr++;

      switch (*formatItr) {
      case 'Y': // year with century
        if (!consumeInteger(4, &year_))
          return false;
        break;
      case 'y': // year without century
        if (!consumeInteger(2, &year_))
          return false;
        year_ += (year_ < 69) ? 2000 : 1900;
        break;
      case 'm': // month
        if (!consumeInteger1length1_or_2(2, &mon_, false))
          return false;
        break;
      case 'b': // abbreviated month name
        if (!consumeString(pLocale_->monAb_, &mon_))
          return false;
        break;
      case 'B': // month name
        if (!consumeString(pLocale_->mon_, &mon_))
          return false;
        break;
      case 'd': // day
        if (!consumeInteger1length1_or_2(2, &day_, false))
          return false;
        break;
      case 'a': // abbreviated day of week
        if (!consumeString(pLocale_->dayAb_, &day_))
          return false;
        break;
      case 'e': // day with optional leading space
        if (!consumeInteger1WithSpace(2, &day_))
          return false;
        break;
      case 'H': // hour
        if (!consumeInteger(2, &hour_, false))
          return false;
        break;
      case 'I': // hour
        if (!consumeInteger(2, &hour_, false))
          return false;
        if (hour_ < 1 || hour_ > 12) {
          return false;
        }
        hour_ %= 12;
        break;
      case 'M': // minute
        if (!consumeInteger(2, &min_))
          return false;
        break;
      case 'S': // seconds (integer)
        if (!consumeSeconds(&sec_, NULL))
          return false;
        break;
      case 'O': // seconds (double)
        if (formatItr + 1 == formatEnd || *(formatItr + 1) != 'S')
          Rcpp::stop("Invalid format: %%O must be followed by %%S");
        formatItr++;
        if (!consumeSeconds(&sec_, &psec_))
          return false;
        break;

      case 'p': // AM/PM
        if (!consumeString(pLocale_->amPm_, &amPm_))
          return false;
        break;

      case 'z': // time zone specification
        tz_ = "UTC";
        if (!consumeTzOffset(&tzOffsetHours_, &tzOffsetMinutes_))
          return false;
        break;
      case 'Z': // time zone name
        if (!consumeTzName(&tz_))
          return false;
        break;

      // Extensions
      case '.':
        if (!consumeNonDigit())
          return false;
        break;

      case '+':
        if (!consumeNonDigits())
          return false;
        break;

      case '*':
        consumeNonDigits();
        break;

      case 'A': // auto date / time
        if (formatItr + 1 == formatEnd)
          Rcpp::stop("Invalid format: %%A must be followed by another letter");
        formatItr++;
        switch (*formatItr) {
        case 'D':
          if (!parseDate())
            return false;
          break;
        case 'T':
          if (!parseTime())
            return false;
          break;
        default:
          Rcpp::stop("Invalid %%A auto parser");
        }
        break;

      // Compound formats
      case 'D':
        parse("%m/%d/%y");
        break;
      case 'F':
        parse("%Y-%m-%d");
        break;
      case 'R':
        parse("%H:%M");
        break;
      case 'X':
      case 'T':
        parse("%H:%M:%S");
        break;
      case 'x':
        parse("%y/%m/%d");
        break;

      default:
        Rcpp::stop("Unsupported format %%%s", *formatItr);
      }
    }

    consumeWhiteSpace(); // always consume trailing whitespace

    return isComplete();
  }

  DateTime makeDateTime() {
    DateTime dt(year_, mon_, day_, hour(), min_, sec_, psec_, tz_);
    if (tz_ == "UTC")
      dt.setOffset(-tzOffsetHours_ * 3600 - tzOffsetMinutes_ * 60);

    return dt;
  }
  DateTime makeDate() {
    DateTime dt(year_, mon_, day_, 0, 0, 0, 0, "UTC");
    return dt;
  }
  DateTime makeTime() {
    DateTime dt(0, 0, 0, hour(), min_, sec_, psec_, "UTC");
    return dt;
  }

  bool compactDate() { return compactDate_; }

  int year() { return year_; }

private:
  int hour() {
    if (hour_ == 12) {

      // 12 AM
      if (amPm_ == 0) {
        return hour_ - 12;
      }

      // 12 PM
      return hour_;
    }

    // Rest of PM
    if (amPm_ == 1) {
      return hour_ + 12;
    }

    // 24 hour time
    return hour_;
  }

  inline bool consumeSeconds(int* pSec, double* pPartialSec) {
    double sec;
    if (!consumeDouble(&sec))
      return false;

    *pSec = (int)sec;
    if (pPartialSec != NULL)
      *pPartialSec = sec - *pSec;
    return true;
  }

  inline bool
  consumeString(const std::vector<std::string>& haystack, int* pOut) {
    // haystack is always in UTF-8
    std::string needleUTF8 = pLocale_->encoder_.makeString(dateItr_, dateEnd_);
    std::transform(
        needleUTF8.begin(), needleUTF8.end(), needleUTF8.begin(), ::tolower);

    for (size_t i = 0; i < haystack.size(); ++i) {
      std::string hay = haystack[i];
      std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
      if (needleUTF8.find(hay) != std::string::npos) {
        *pOut = i;
        dateItr_ += hay.size();
        return true;
      }
    }

    return false;
  }

  inline bool consumeInteger(int n, int* pOut, bool exact = true) {
    if (dateItr_ == dateEnd_ || *dateItr_ == '-' || *dateItr_ == '+')
      return false;

    const char* start = dateItr_;
    const char* end = std::min(dateItr_ + n, dateEnd_);
    bool ok = parseInt(dateItr_, end, *pOut);

    return ok && (!exact || (dateItr_ - start) == n);
  }

  // Integer indexed from 1 (i.e. month and date)
  inline bool consumeInteger1(int n, int* pOut, bool exact = true) {
    if (!consumeInteger(n, pOut, exact))
      return false;

    (*pOut)--;
    return true;
  }

  // Integer indexed from 1 (i.e. month and date) which can take 1 or 2
  // positions
  inline bool consumeInteger1length1_or_2(int n, int* pOut, bool exact = true) {
    int out1, out2;
    if (!consumeInteger(1, &out1, true))
      return false;
    else {
      if (consumeInteger(1, &out2, true))
        *pOut = 10 * out1 + out2;
      else {
        *pOut = out1;
        dateItr_--; // unconsume the last read non-integer char
      }
    }

    (*pOut)--;
    return true;
  }

  // Integer indexed from 1 with optional space
  inline bool consumeInteger1WithSpace(int n, int* pOut) {
    if (consumeThisChar(' '))
      n--;

    return consumeInteger1(n, pOut);
  }

  inline bool consumeDouble(double* pOut) {
    if (dateItr_ == dateEnd_ || *dateItr_ == '-' || *dateItr_ == '+')
      return false;
    return parseDouble(pLocale_->decimalMark_, dateItr_, dateEnd_, *pOut);
  }

  inline bool consumeWhiteSpace() {
    while (dateItr_ != dateEnd_ && std::isspace(*dateItr_))
      dateItr_++;

    return true;
  }

  inline bool consumeNonDigit() {
    if (dateItr_ == dateEnd_ || std::isdigit(*dateItr_))
      return false;

    dateItr_++;
    return true;
  }

  inline bool consumeNonDigits() {
    if (!consumeNonDigit())
      return false;

    while (dateItr_ != dateEnd_ && !std::isdigit(*dateItr_))
      dateItr_++;

    return true;
  }

  inline bool consumeChar(char* pOut) {
    if (dateItr_ == dateEnd_)
      return false;

    *pOut = *dateItr_++;
    return true;
  }

  inline bool consumeThisChar(char needed) {
    if (dateItr_ == dateEnd_ || *dateItr_ != needed)
      return false;

    dateItr_++;
    return true;
  }

  inline bool consumeAMPM(bool* pIsPM) {
    if (dateItr_ == dateEnd_)
      return false;

    if (consumeThisChar('A') || consumeThisChar('a')) {
      *pIsPM = false;
    } else if (consumeThisChar('P') || consumeThisChar('p')) {
      *pIsPM = true;
    } else {
      return false;
    }

    if (!(consumeThisChar('M') || consumeThisChar('m')))
      return false;

    return true;
  }

  // ISO8601 style
  // Z
  // ±hh:mm
  // ±hhmm
  // ±hh
  inline bool consumeTzOffset(int* pHours, int* pMinutes) {
    if (consumeThisChar('Z'))
      return true;

    // Optional +/- (required for ISO8601 but we'll let it slide)
    int mult = 1;
    if (*dateItr_ == '+' || *dateItr_ == '-') {
      mult = (*dateItr_ == '-') ? -1 : 1;
      dateItr_++;
    }

    // Required hours
    if (!consumeInteger(2, pHours))
      return false;

    // Optional colon and minutes
    consumeThisChar(':');
    consumeInteger(2, pMinutes);

    *pHours *= mult;
    *pMinutes *= mult;

    return true;
  }

  inline bool consumeTzName(std::string* pOut) {
    const char* tzStart = dateItr_;
    while (dateItr_ != dateEnd_ && !std::isspace(*dateItr_))
      dateItr_++;

    pOut->assign(tzStart, dateItr_);
    return tzStart != dateItr_;
  }

  void reset() {
    year_ = -1;
    mon_ = 0;
    day_ = 0;
    hour_ = 0;
    min_ = 0;
    sec_ = 0;
    psec_ = 0;
    amPm_ = -1;
    compactDate_ = true;

    tzOffsetHours_ = 0;
    tzOffsetMinutes_ = 0;
    tz_ = tzDefault_;
  }
};

#endif
