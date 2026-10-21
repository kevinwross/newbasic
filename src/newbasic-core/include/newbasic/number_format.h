#pragma once

#include <string>

#include "newbasic/value.h"

namespace newbasic {

// Formats the numeric value for PRINT output. Returns the exact printed
// digits/sign WITHOUT the trailing space (the VM appends that space per the
// guide: "the numbers are always followed by a space").
//
// Documented rules (GW-BASIC User's Reference, PRINT; User's Guide ch. 6):
//  - Positive numbers are preceded by a space, negative by '-'.
//  - Int/Long: plain decimal digits.
//  - Sgl: seven significant digits; Dbl: sixteen significant digits
//    (guide: doubles are stored with 17 digits, printed with as many as 16).
//  - Fixed-point form is used while the digit budget suffices
//    (Sgl: 1e-6 <= |v| < 1e7; Dbl: 1e-15 <= |v| < 1e16); otherwise
//    exponential "d[.ddd]E+xx" with the full significant-digit budget.
//    (Exact GW-BASIC fixed/exponent threshold is inferred from the digit
//    budget; see plan.md known divergences.)
//  - No leading zero before the decimal point (".5"); trailing zeros are
//    stripped; negative zero prints as positive.
//  - Rounding: correctly rounded decimal conversion (round-to-nearest,
//    ties-to-even), matching C library printf semantics.
std::string FormatNumber(const Value& v);

// Formats a string value for PRINT: the raw characters (CP437 bytes) as-is,
// no leading space, no trailing space.
std::string FormatString(const Value& v);

// Formats any value the way PRINT does (numeric -> FormatNumber, string ->
// FormatString).
std::string FormatForPrint(const Value& v);

} // namespace newbasic
