#pragma once

#include <cstdint>
#include <string>

#include "newbasic/mbf.h"

namespace newbasic {

// M9: binary field conversions used by random-access files (and by FIELD-typed
// record data). The encodings are the ones GW-BASIC uses on disk:
//   MKI$/CVI  2-byte signed integer, low byte first
//   MKS$/CVS  4-byte MBF single precision
//   MKD$/CVD  8-byte MBF double precision
// The MBF codec is shared by the numeric field conversions.

inline std::string MkiFromInt(std::int16_t value) {
  std::string out(2, '\0');
  out[0] = static_cast<char>(static_cast<std::uint8_t>(value & 0xFF));
  out[1] = static_cast<char>(static_cast<std::uint8_t>((value >> 8) & 0xFF));
  return out;
}

// Returns false when the string is not exactly two bytes long (Illegal function
// call at the language level).
inline bool CviFromString(const std::string& text, std::int16_t* out) {
  if (text.size() != 2)
    return false;
  const std::uint16_t low = static_cast<unsigned char>(text[0]);
  const std::uint16_t high = static_cast<unsigned char>(text[1]);
  *out = static_cast<std::int16_t>(static_cast<std::uint16_t>(low | (high << 8)));
  return true;
}

inline std::string MksFromSingle(double value) {
  std::uint8_t raw[4] = {0, 0, 0, 0};
  mbf::EncodeMbfSingle(value, raw);
  return std::string(reinterpret_cast<const char*>(raw), sizeof(raw));
}

inline bool CvsFromString(const std::string& text, double* out) {
  if (text.size() != 4)
    return false;
  *out = mbf::DecodeMbfSingle(reinterpret_cast<const std::uint8_t*>(text.data()));
  return true;
}

inline std::string MkdFromDouble(double value) {
  std::uint8_t raw[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  mbf::EncodeMbfDouble(value, raw);
  return std::string(reinterpret_cast<const char*>(raw), sizeof(raw));
}

inline bool CvdFromString(const std::string& text, double* out) {
  if (text.size() != 8)
    return false;
  *out = mbf::DecodeMbfDouble(reinterpret_cast<const std::uint8_t*>(text.data()));
  return true;
}

} // namespace newbasic
