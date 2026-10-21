#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace lwbasic::fstok {

// Protected .BAS files (the 0xFE format, M4): the tokenized format with the
// envelope byte changed from 0xFF to 0xFE and the remainder transformed by
// Kocher's byte algorithm.
//
// Keys and transform are interface facts sourced from PC-BASIC's protect.py
// (GPL-3.0), which cites Paul Kocher's reverse engineering published in The
// Cryptogram computer supplement #19 (1994). No code was copied; see
// docs/m4-protected-files.md for the full provenance and for an important
// caveat: a round-trip test proves that ProtectTokenized and UnprotectTokenized
// are mutual inverses, NOT that we agree byte-for-byte with Microsoft's own
// files, because the index offset and the envelope/trailer handling are
// inferred from the published description.

// The 13-byte and 11-byte keys. 13 and 11 are coprime, so the pair repeats only
// after 143 bytes, which is what makes the scheme non-trivial.
inline const std::uint8_t* ProtectKey1() {
  static const std::uint8_t key[13] = {0xA9, 0x84, 0x8D, 0xCD, 0x75, 0x83, 0x43,
                                       0x63, 0x24, 0x83, 0x19, 0xF7, 0x9A};
  return key;
}

inline const std::uint8_t* ProtectKey2() {
  static const std::uint8_t key[11] = {0x1E, 0x1D, 0xC4, 0x77, 0x26, 0x97,
                                       0xE0, 0x74, 0x59, 0x88, 0x7C};
  return key;
}

// One byte of the transformation for position i in 0..142.
inline std::uint8_t ProtectByte(std::uint8_t value, int index) {
  const std::uint8_t* key1 = ProtectKey1();
  const std::uint8_t* key2 = ProtectKey2();
  int byte = (static_cast<int>(value) - (13 - index % 13)) % 256;
  if (byte < 0)
    byte += 256;
  byte ^= key1[index % 13];
  byte ^= key2[index % 11];
  byte = (byte + (11 - index % 11)) % 256;
  return static_cast<std::uint8_t>(byte);
}

inline std::uint8_t UnprotectByte(std::uint8_t value, int index) {
  const std::uint8_t* key1 = ProtectKey1();
  const std::uint8_t* key2 = ProtectKey2();
  int byte = (static_cast<int>(value) - (11 - index % 11)) % 256;
  if (byte < 0)
    byte += 256;
  byte ^= key1[index % 13];
  byte ^= key2[index % 11];
  byte = (byte + (13 - index % 13)) % 256;
  return static_cast<std::uint8_t>(byte);
}

// Tokenized program (0xFF envelope) -> protected file (0xFE envelope). The
// envelope itself is not transformed; the rest is, with the index starting at
// the first byte after it.
inline std::string ProtectTokenized(const std::string& tokenized) {
  if (tokenized.empty())
    return tokenized;
  std::string out = tokenized;
  out[0] = static_cast<char>(0xFE);
  for (std::size_t i = 1; i + 1 < out.size(); ++i) {
    const int index = static_cast<int>((i - 1) % 143);
    out[i] = static_cast<char>(ProtectByte(static_cast<std::uint8_t>(out[i]), index));
  }
  return out;
}

// Protected file -> tokenized program, ready for DecodeTokenized.
inline std::string UnprotectTokenized(const std::string& protected_bytes) {
  if (protected_bytes.empty())
    return protected_bytes;
  std::string out = protected_bytes;
  out[0] = static_cast<char>(0xFF);
  for (std::size_t i = 1; i + 1 < out.size(); ++i) {
    const int index = static_cast<int>((i - 1) % 143);
    out[i] = static_cast<char>(UnprotectByte(static_cast<std::uint8_t>(out[i]), index));
  }
  return out;
}

} // namespace lwbasic::fstok
