#include "lwbasic/using.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace lwbasic {
namespace {

bool IsNumericFieldStart(const std::string& f, std::size_t i) {
  const char c = f[i];
  if (c == '#' || c == '.' || c == '+' || c == '-')
    return true;
  if (c == '$' && i + 1 < f.size() && f[i + 1] == '$')
    return true;
  if (c == '*' && i + 1 < f.size() && f[i + 1] == '*')
    return true;
  return false;
}

bool IsStringFieldStart(char c) {
  return c == '!' || c == '&' || c == '\\';
}

// Scans a numeric field starting at i and returns its text, advancing *pos.
std::string ScanNumericField(const std::string& f, std::size_t* pos) {
  std::string field;
  std::size_t i = *pos;
  if (i < f.size() && f[i] == '+')
    field += f[i++];
  if (i + 1 < f.size() && f[i] == '*' && f[i + 1] == '*') {
    field += "**";
    i += 2;
    if (i < f.size() && f[i] == '$')
      field += f[i++];
  } else if (i + 1 < f.size() && f[i] == '$' && f[i + 1] == '$') {
    field += "$$";
    i += 2;
  }
  while (i < f.size() && (f[i] == '#' || f[i] == ','))
    field += f[i++];
  if (i < f.size() && f[i] == '.') {
    field += f[i++];
    while (i < f.size() && (f[i] == '#' || f[i] == ','))
      field += f[i++];
  }
  if (i + 3 < f.size() && f.compare(i, 4, "^^^^") == 0) {
    field += "^^^^";
    i += 4;
  }
  if (i < f.size() && (f[i] == '-' || f[i] == '+'))
    field += f[i++];
  *pos = i;
  return field;
}

// Scans a string field starting at i and returns its text, advancing *pos.
std::string ScanStringField(const std::string& f, std::size_t* pos) {
  std::string field;
  std::size_t i = *pos;
  if (f[i] == '!' || f[i] == '&') {
    field += f[i++];
  } else { // backslash-delimited string field: the width includes both delimiters
    field += f[i++];
    while (i < f.size() && f[i] != '\\')
      field += f[i++];
    if (i < f.size() && f[i] == '\\')
      field += f[i++];
  }
  *pos = i;
  return field;
}

int CountDigits(const std::string& s, std::size_t begin, std::size_t end) {
  int count = 0;
  for (std::size_t i = begin; i < end && i < s.size(); ++i) {
    if (s[i] == '#')
      ++count;
  }
  return count;
}

// Inserts grouping commas every three digits from the right.
std::string GroupThousands(const std::string& digits) {
  std::string grouped;
  int since_comma = 0;
  for (std::size_t n = digits.size(); n-- > 0;) {
    grouped.insert(grouped.begin(), digits[n]);
    if (++since_comma == 3 && n > 0) {
      grouped.insert(grouped.begin(), ',');
      since_comma = 0;
    }
  }
  return grouped;
}

std::string FixedPoint(double value, int decimals) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
  return std::string(buffer);
}

} // namespace

UsingStatus FormatUsingNumber(std::string* out, const std::string& field, double value) {
  // --- field anatomy -------------------------------------------------------
  // Prefixes (the reference: `**` fills with asterisks and adds two digit
  // positions, `$$` puts a dollar sign immediately before the number and adds two,
  // `**$` does both and adds three). The field width is the field text length; the
  // prefix columns double as digit positions.
  std::size_t body_start = 0;
  int extra_positions = 0;
  bool asterisk_fill = false;
  bool dollar = false;
  if (field.compare(0, 3, "**$") == 0) {
    asterisk_fill = true;
    dollar = true;
    extra_positions = 3;
    body_start = 3;
  } else if (field.compare(0, 2, "**") == 0) {
    asterisk_fill = true;
    extra_positions = 2;
    body_start = 2;
  } else if (field.compare(0, 2, "$$") == 0) {
    dollar = true;
    extra_positions = 2;
    body_start = 2;
  }
  const bool leading_plus = body_start < field.size() && field[body_start] == '+';
  if (leading_plus)
    ++body_start;

  const bool exponential =
      field.size() >= body_start + 4 && field.compare(field.size() - 4, 4, "^^^^") == 0;
  const std::size_t body_end = exponential ? field.size() - 4 : field.size();
  const bool trailing_sign =
      body_end > body_start && (field[body_end - 1] == '-' || field[body_end - 1] == '+');
  const std::size_t body_stop = body_end - (trailing_sign ? 1 : 0);
  const std::string body = field.substr(body_start, body_stop - body_start);

  const std::size_t dot = body.find('.');
  const std::size_t int_end = dot == std::string::npos ? body.size() : dot;
  const int int_places = CountDigits(body, 0, int_end) + extra_positions;
  const int frac_places = dot == std::string::npos ? 0 : CountDigits(body, dot + 1, body.size());
  const bool grouping =
      dot != std::string::npos && body.find(',') != std::string::npos && body.find(',') < dot;
  const std::size_t width = field.size();
  const bool negative = value < 0.0;
  const char fill = asterisk_fill ? '*' : ' ';

  // --- the digits ----------------------------------------------------------
  std::string digits;
  if (exponential) {
    // Interpretation: one sign column, one digit before the point, `frac_places`
    // after it, then `E±dd`; the exponent sign is always printed.
    double magnitude = std::fabs(value);
    int exponent = 0;
    if (magnitude > 0.0) {
      exponent = static_cast<int>(std::floor(std::log10(magnitude)));
      magnitude /= std::pow(10.0, exponent);
      // Rounding may carry into the next decade (9.99 -> 10.0).
      char probe[64];
      std::snprintf(probe, sizeof(probe), "%.*f", frac_places, magnitude);
      if (std::stod(probe) >= 10.0) {
        magnitude /= 10.0;
        ++exponent;
      }
    }
    char mantissa[64];
    std::snprintf(mantissa, sizeof(mantissa), "%.*f", frac_places, magnitude);
    char exponent_text[16];
    std::snprintf(exponent_text, sizeof(exponent_text), "E%c%02d", exponent < 0 ? '-' : '+',
                  std::abs(exponent) % 100);
    const char sign = leading_plus ? (negative ? '-' : '+') : (negative ? '-' : ' ');
    digits = std::string(1, sign) + mantissa + exponent_text;
    if (digits.size() > width) {
      *out += '%';
      *out += digits;
      return UsingStatus::Ok;
    }
    out->append(width - digits.size(), fill);
    *out += digits;
    return UsingStatus::Ok;
  }

  double magnitude = std::fabs(value);
  const double scale = std::pow(10.0, frac_places);
  magnitude = std::round(magnitude * scale) / scale;

  digits = FixedPoint(magnitude, frac_places);
  if (frac_places == 0) {
    const std::size_t point = digits.find('.');
    if (point != std::string::npos)
      digits.erase(point);
    // A format that ends at the point ("###.") still prints the point.
    if (dot != std::string::npos)
      digits += '.';
  }
  if (int_places == 0 && !digits.empty() && digits[0] == '0' && dot != std::string::npos) {
    // A format with no digit position before the point (".##") prints just the
    // fraction, without a leading zero.
    digits.erase(0, 1);
  }
  if (grouping) {
    const std::size_t point = digits.find('.');
    const std::string whole = digits.substr(0, point);
    const std::string rest = point == std::string::npos ? std::string() : digits.substr(point);
    digits = GroupThousands(whole) + rest;
  }
  // The dollar sign sits immediately left of the digits.
  if (dollar)
    digits = "$" + digits;

  std::string core;
  if (leading_plus) {
    core = std::string(1, negative ? '-' : '+') + digits;
  } else if (trailing_sign) {
    const char trailing = negative ? '-' : (field.back() == '+' ? '+' : ' ');
    core = digits + std::string(1, trailing);
  } else if (negative && dollar) {
    // Decision: the reference says a negative cannot be used with `$$` unless the
    // minus trails; rather than refuse, the minus goes before the dollar sign.
    core = "-" + digits;
  } else if (digits.size() < width) {
    // The default sign position is used only when it fits: the reference prints
    // 987.65 in a "###.##" field, i.e. a value that fills the field prints
    // without a leading space. With asterisk fill the position is filled with '*',
    // which is how the reference example shows "*12.4".
    core = std::string(1, negative ? '-' : fill) + digits;
  } else {
    core = digits;
  }

  if (core.size() > width) {
    // The reference prints a percent sign and then the number, unpadded, when it
    // does not fit the field.
    *out += '%';
    if (negative && !dollar)
      *out += '-';
    *out += digits;
    return UsingStatus::Ok;
  }
  out->append(width - core.size(), fill);
  *out += core;
  return UsingStatus::Ok;
}

UsingStatus FormatUsingString(std::string* out, const std::string& field,
                              const std::string& value) {
  if (field == "!") {
    out->push_back(value.empty() ? ' ' : value[0]);
    return UsingStatus::Ok;
  }
  if (field == "&") {
    *out += value; // variable length: exactly as given
    return UsingStatus::Ok;
  }
  // \n spaces\: 2 + n characters, where n is the number of characters between the
  // backslashes.
  const std::size_t width = field.size(); // includes both backslashes
  std::string taken = value.substr(0, width);
  *out += taken;
  out->append(width - taken.size(), ' '); // left-justified, padded right
  return UsingStatus::Ok;
}

UsingStatus AppendUsing(std::string* out, const std::string& format, std::size_t* pos,
                        const Value& value) {
  if (format.empty())
    return UsingStatus::TypeMismatch;
  // Each value consumes one field; literals between fields are printed as-is and
  // the format cycles when exhausted. The bound below is a guard against a
  // format made only of literals, which would never find a field.
  const std::size_t limit = 2 * format.size() + 8;
  for (std::size_t step = 0; step < limit; ++step) {
    if (*pos >= format.size())
      *pos = 0; // cycle
    const std::size_t i = *pos;
    const char c = format[i];

    if (IsStringFieldStart(c)) {
      const std::string field = ScanStringField(format, pos);
      if (!value.IsString())
        return UsingStatus::TypeMismatch;
      return FormatUsingString(out, field, value.AsString());
    }
    if (IsNumericFieldStart(format, i)) {
      const std::string field = ScanNumericField(format, pos);
      if (value.IsString())
        return UsingStatus::TypeMismatch;
      // A bare '-' or a stray ',' is a literal, not a field.
      if (field == "-" || field.find('#') == std::string::npos) {
        *out += field;
        continue;
      }
      double number = 0.0;
      if (value.IsNumeric())
        number = value.AsNumber();
      return FormatUsingNumber(out, field, number);
    }
    // `_` prints the character that follows it, so a format can contain a field
    // character literally. Note that literals trailing the last field of a cycle are
    // emitted with the next value, which is why a single item does not show them.
    if (c == '_' && i + 1 < format.size()) {
      *out += format[i + 1];
      *pos = i + 2;
      continue;
    }
    *out += c; // literal
    ++*pos;
  }
  return UsingStatus::TypeMismatch;
}

} // namespace lwbasic
