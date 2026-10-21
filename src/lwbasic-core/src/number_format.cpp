#include "lwbasic/number_format.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace lwbasic {
namespace {

std::string SignPrefix(bool negative) {
  return negative ? std::string("-") : std::string(" ");
}

// Two-digit (or wider for >= 100) GW-BASIC exponent: "E+07", "E-05".
std::string FormatExponent(int exponent) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "E%c%02d", exponent >= 0 ? '+' : '-', std::abs(exponent));
  return std::string(buf);
}

// Core numeric display: `a` > 0, `sig` significant digits (7 for Single,
// 16 for Double), sign applied by the caller.
std::string FormatPositive(double a, int sig, bool negative) {
  int exp10 = static_cast<int>(std::floor(std::log10(a)));
  while (std::pow(10.0, exp10) > a)
    --exp10;
  while (std::pow(10.0, exp10 + 1) <= a)
    ++exp10;

  const double kFixedLo = std::pow(10.0, -(sig - 1));
  const double kFixedHi = std::pow(10.0, sig);
  bool fixed = a >= kFixedLo && a < kFixedHi;

  if (fixed) {
    // `sig - 1 - exp10` decimal places yields exactly `sig` significant
    // digits whether the value is >= 1 or a fraction (0.0196247 needs 8
    // decimals for 7 significant digits).
    const int decimals = sig - 1 - exp10;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, a);
    std::string s(buf);
    if (s.find('.') != std::string::npos) {
      while (!s.empty() && s.back() == '0')
        s.pop_back();
      if (!s.empty() && s.back() == '.')
        s.pop_back();
    }
    // Rounding carry may have overflowed the digit budget
    // (e.g. 9999999.6 -> "10000000"): fall back to exponential. The zero
    // before the point in "0.333" is not a significant digit.
    const bool leading_zero = s.size() >= 2 && s[0] == '0' && s[1] == '.';
    std::string digits = s;
    if (leading_zero)
      digits.erase(0, 1);
    digits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());
    if (digits.size() > static_cast<std::size_t>(sig)) {
      fixed = false;
    } else {
      if (leading_zero)
        s = s.substr(1);
      return SignPrefix(negative) + s;
    }
  }

  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.*e", sig - 1, a);
  std::string s(buf); // e.g. "2.35988e-05"
  const std::size_t epos = s.find('e');
  std::string mant = s.substr(0, epos);
  const int exponent = std::atoi(s.c_str() + epos + 1);
  while (mant.size() > 1 && mant.back() == '0')
    mant.pop_back();
  if (!mant.empty() && mant.back() == '.')
    mant.pop_back();
  return SignPrefix(negative) + mant + FormatExponent(exponent);
}

} // namespace

std::string FormatNumber(const Value& v) {
  switch (v.kind()) {
  case Value::Kind::Int: {
    // The sign prefix is separate, so print the magnitude (avoids "--5").
    const int n = static_cast<int>(v.AsNumber());
    return SignPrefix(n < 0) + std::to_string(n < 0 ? -n : n);
  }
  case Value::Kind::Long: {
    const long long n = static_cast<long long>(v.AsNumber());
    return SignPrefix(n < 0) + std::to_string(n < 0 ? -n : n);
  }
  case Value::Kind::Sgl:
    if (v.AsNumber() == 0.0)
      return " 0";
    return FormatPositive(std::fabs(v.AsNumber()), 7, v.AsNumber() < 0.0);
  case Value::Kind::Dbl:
    if (v.AsNumber() == 0.0)
      return " 0";
    return FormatPositive(std::fabs(v.AsNumber()), 16, v.AsNumber() < 0.0);
  case Value::Kind::Str:
    break;
  }
  return {};
}

std::string FormatString(const Value& v) {
  (void)v;
  // Strings print their raw characters (CP437 bytes) with no padding.
  return v.AsString();
}

std::string FormatForPrint(const Value& v) {
  return v.IsString() ? FormatString(v) : FormatNumber(v);
}

} // namespace lwbasic
