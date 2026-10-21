#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "newbasic/fstok/mbf.h"
#include "newbasic/fstok/token_table.h"

namespace newbasic::fstok {

// Tokenized GW-BASIC program decoding (M4).
//
// File layout (sourced - see docs/m4-tokenized-format-notes.md):
//   [0x00] envelope: 0xFF unprotected, 0xFE protected
//   then, per line: [2 bytes next-line offset, LSB first] [2 bytes line
//   number, LSB first] [tokenized text] [0x00], a 0x0000 offset ending the
//   program, and a trailing 0x1A.
// Offsets are recomputed when a file is loaded, so only their relative
// differences matter; this decoder treats them as offsets from the file start
// and validates that they advance.
//
// Decoding only for now: the encoder is the next step, and protected files
// (the XOR scheme) are still to be sourced.

struct DecodedLine {
  int number = 0;
  std::string source;
};

namespace detail {

inline unsigned Read16(const std::string& bytes, std::size_t pos) {
  return static_cast<unsigned>(static_cast<unsigned char>(bytes[pos])) |
         (static_cast<unsigned>(static_cast<unsigned char>(bytes[pos + 1])) << 8);
}

inline std::string FormatSingle(double value) {
  char buffer[40];
  std::snprintf(buffer, sizeof(buffer), "%.9g", value);
  return buffer;
}

inline std::string FormatDouble(double value) {
  char buffer[40];
  std::snprintf(buffer, sizeof(buffer), "%.17g", value);
  return buffer;
}

} // namespace detail

// Decodes a tokenized program (including its envelope byte) into text lines.
// Returns false and sets `error` on malformed input rather than dropping bytes.
inline bool DecodeTokenized(const std::string& bytes, std::vector<DecodedLine>* lines,
                            std::string* error) {
  auto fail = [error](const std::string& reason) {
    if (error != nullptr)
      *error = reason;
    return false;
  };
  lines->clear();
  if (bytes.empty())
    return fail("empty file");
  const unsigned char envelope = static_cast<unsigned char>(bytes[0]);
  if (envelope == 0xFE)
    return fail("protected program");
  if (envelope != 0xFF)
    return fail("not a tokenized program (expected a 0xFF envelope)");

  std::size_t pos = 1;
  while (true) {
    if (pos + 2 > bytes.size())
      return fail("truncated line offset");
    const unsigned offset = detail::Read16(bytes, pos);
    if (offset == 0)
      break; // end of program
    pos += 2;
    if (pos + 2 > bytes.size())
      return fail("truncated line number");
    const int number = static_cast<int>(detail::Read16(bytes, pos));
    pos += 2;

    std::string source;
    bool terminated = false;
    while (pos < bytes.size()) {
      const unsigned char byte = static_cast<unsigned char>(bytes[pos]);
      if (byte == 0x00) {
        ++pos;
        terminated = true;
        break;
      }
      // Printable ASCII stands for itself.
      if (byte >= 0x20 && byte <= 0x7E) {
        source += static_cast<char>(byte);
        ++pos;
        continue;
      }
      // Constants 0..10 have their own tokens.
      if (byte >= 0x11 && byte <= 0x1B) {
        source += std::to_string(byte - 0x11);
        ++pos;
        continue;
      }
      // Two-byte keyword tokens.
      if (byte == 0xFD || byte == 0xFE || byte == 0xFF) {
        if (pos + 1 >= bytes.size())
          return fail("truncated two-byte token");
        const std::uint16_t code =
            static_cast<std::uint16_t>((byte << 8) | static_cast<unsigned char>(bytes[pos + 1]));
        const std::optional<std::string> keyword = KeywordForCode(code);
        if (!keyword.has_value())
          return fail("unknown two-byte token");
        source += *keyword;
        pos += 2;
        continue;
      }
      // Single-byte keyword tokens.
      if (byte >= 0x81) {
        const std::optional<std::string> keyword = KeywordForCode(byte);
        if (!keyword.has_value()) {
          char message[48];
          std::snprintf(message, sizeof(message), "undefined token 0x%02X", byte);
          return fail(message);
        }
        source += *keyword;
        ++pos;
        continue;
      }
      // Numeric constants.
      switch (byte) {
      case 0x0B:
      case 0x0C:
      case 0x0E:
      case 0x1C: {
        if (pos + 3 > bytes.size())
          return fail("truncated two-byte constant");
        const unsigned value = detail::Read16(bytes, pos + 1);
        if (byte == 0x0B) {
          // Fixed width. A 16-bit octal constant is at most six digits, and padding
          // to that width keeps a following digit from extending it when the text is
          // re-parsed: "&O36271" followed by "0" would otherwise re-encode as the
          // six-digit "&O362710" (124360, truncated), which is a different value.
          char buffer[24];
          std::snprintf(buffer, sizeof(buffer), "&O%06o", value);
          source += buffer;
        } else if (byte == 0x0C) {
          // Likewise four digits for hex.
          char buffer[24];
          std::snprintf(buffer, sizeof(buffer), "&H%04X", value);
          source += buffer;
        } else if (byte == 0x0E) {
          source += std::to_string(value); // a line number reference
        } else {
          source += std::to_string(static_cast<int>(static_cast<std::int16_t>(value)));
        }
        pos += 3;
        continue;
      }
      case 0x0F: {
        if (pos + 2 > bytes.size())
          return fail("truncated byte constant");
        source += std::to_string(static_cast<unsigned char>(bytes[pos + 1]));
        pos += 2;
        continue;
      }
      case 0x1D: {
        if (pos + 5 > bytes.size())
          return fail("truncated single-precision constant");
        const std::uint8_t raw[4] = {
            static_cast<std::uint8_t>(bytes[pos + 1]), static_cast<std::uint8_t>(bytes[pos + 2]),
            static_cast<std::uint8_t>(bytes[pos + 3]), static_cast<std::uint8_t>(bytes[pos + 4])};
        source += detail::FormatSingle(DecodeMbfSingle(raw));
        pos += 5;
        continue;
      }
      case 0x1F: {
        if (pos + 9 > bytes.size())
          return fail("truncated double-precision constant");
        const std::uint8_t raw[8] = {
            static_cast<std::uint8_t>(bytes[pos + 1]), static_cast<std::uint8_t>(bytes[pos + 2]),
            static_cast<std::uint8_t>(bytes[pos + 3]), static_cast<std::uint8_t>(bytes[pos + 4]),
            static_cast<std::uint8_t>(bytes[pos + 5]), static_cast<std::uint8_t>(bytes[pos + 6]),
            static_cast<std::uint8_t>(bytes[pos + 7]), static_cast<std::uint8_t>(bytes[pos + 8])};
        source += detail::FormatDouble(DecodeMbfDouble(raw));
        pos += 9;
        continue;
      }
      default:
        break;
      }
      char message[48];
      std::snprintf(message, sizeof(message), "unsupported token 0x%02X", byte);
      return fail(message);
    }
    if (!terminated)
      return fail("unterminated line");
    DecodedLine line;
    line.number = number;
    line.source = source;
    lines->push_back(std::move(line));
  }
  return true;
}

} // namespace newbasic::fstok
