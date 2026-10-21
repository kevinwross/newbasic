#pragma once

#include <cmath>
#include <cstdint>

namespace lwbasic::fstok {

// Microsoft Binary Format (MBF), the numeric encoding used inside a tokenized
// GW-BASIC program (M4). MBF is *not* IEEE 754: there is an implied leading 1
// bit, the sign lives in the top bit of the first mantissa byte, the exponent
// comes first and is biased by 128, and zero is all-zero bytes. No denormals,
// infinities or NaNs exist.
//
//   single (4 bytes): [exponent] [s|mantissa 22..16] [mantissa 15..8] [7..0]
//   double (8 bytes): [exponent] [s|mantissa 54..48] ... [mantissa 7..0]
//
// Layout reference: "Microsoft Binary Format" (Wikipedia), which documents the
// bit layout as 8-bit exponent, 1-bit sign, 23-bit (single) / 55-bit (double)
// significand with a leading 1 implied and exponent bias 128.
//
// RANGE: the exponent is 8 bits with bias 128, so the representable magnitude
// is roughly 2^-127 to 2^127 (about 5.9e-39 to 1.7e38) for both precisions -
// MBF has no infinities or denormals. Values outside that range are not
// representable in a file; the encoder does not invent a saturation, so callers
// must reject them (GW-BASIC itself would have raised Overflow before saving).
// Note this encoding is about *files*: the interpreter itself uses host
// floating point for arithmetic by project decision (plan.md).

inline void EncodeMbfSingle(double value, std::uint8_t* out) {
  if (value == 0.0 || !std::isfinite(value)) {
    out[0] = out[1] = out[2] = out[3] = 0;
    return;
  }
  const bool negative = value < 0.0;
  double magnitude = std::fabs(value);
  int exponent = 0;
  while (magnitude >= 2.0) {
    magnitude /= 2.0;
    ++exponent;
  }
  while (magnitude < 1.0) {
    magnitude *= 2.0;
    --exponent;
  }
  // Round the 23-bit fraction to nearest.
  std::uint32_t mantissa = static_cast<std::uint32_t>(std::ldexp(magnitude - 1.0, 23) + 0.5);
  if (mantissa > 0x7FFFFFu) { // rounding carried into the implied bit
    mantissa = 0;
    ++exponent;
  }
  out[0] = static_cast<std::uint8_t>(exponent + 128);
  // No explicit leading 1 is stored: bit 7 of the first mantissa byte is the
  // sign, and the 1 before the radix point is implied.
  out[1] = static_cast<std::uint8_t>((negative ? 0x80u : 0u) | (mantissa >> 16));
  out[2] = static_cast<std::uint8_t>((mantissa >> 8) & 0xFFu);
  out[3] = static_cast<std::uint8_t>(mantissa & 0xFFu);
}

inline double DecodeMbfSingle(const std::uint8_t* in) {
  if (in[0] == 0)
    return 0.0; // zero (MBF has no other special cases)
  const bool negative = (in[1] & 0x80u) != 0;
  const std::uint32_t mantissa = (static_cast<std::uint32_t>(in[1] & 0x7Fu) << 16) |
                                 (static_cast<std::uint32_t>(in[2]) << 8) |
                                 static_cast<std::uint32_t>(in[3]);
  const double value =
      std::ldexp(1.0 + static_cast<double>(mantissa) / 8388608.0, static_cast<int>(in[0]) - 128);
  return negative ? -value : value;
}

inline void EncodeMbfDouble(double value, std::uint8_t* out) {
  for (int i = 0; i < 8; ++i)
    out[i] = 0;
  if (value == 0.0 || !std::isfinite(value))
    return;
  const bool negative = value < 0.0;
  double magnitude = std::fabs(value);
  int exponent = 0;
  while (magnitude >= 2.0) {
    magnitude /= 2.0;
    ++exponent;
  }
  while (magnitude < 1.0) {
    magnitude *= 2.0;
    --exponent;
  }
  // 55-bit fraction, held as 56 bits of which the top is the stored 1 bit.
  const long double fraction =
      (static_cast<long double>(magnitude) - 1.0L) * 36028797018963968.0L; // 2^55
  const std::uint64_t mantissa = static_cast<std::uint64_t>(fraction + 0.5L);
  out[0] = static_cast<std::uint8_t>(exponent + 128);
  out[1] = static_cast<std::uint8_t>((negative ? 0x80u : 0u) | ((mantissa >> 48) & 0x7Fu));
  for (int i = 0; i < 6; ++i) {
    out[2 + i] = static_cast<std::uint8_t>((mantissa >> (40 - 8 * i)) & 0xFFu);
  }
}

inline double DecodeMbfDouble(const std::uint8_t* in) {
  if (in[0] == 0)
    return 0.0;
  const bool negative = (in[1] & 0x80u) != 0;
  std::uint64_t mantissa = static_cast<std::uint64_t>(in[1] & 0x7Fu);
  for (int i = 0; i < 6; ++i) {
    mantissa = (mantissa << 8) | static_cast<std::uint64_t>(in[2 + i]);
  }
  const long double value =
      std::ldexp(1.0L + static_cast<long double>(mantissa) / 36028797018963968.0L,
                 static_cast<int>(in[0]) - 128);
  return static_cast<double>(negative ? -value : value);
}

} // namespace lwbasic::fstok
