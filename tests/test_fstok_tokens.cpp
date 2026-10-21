// M4: GW-BASIC tokenized-BASIC token table (the prerequisite for the codec).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <set>
#include <string>

#include "lwbasic/fstok/mbf.h"
#include "lwbasic/fstok/token_table.h"

using Catch::Approx;

using lwbasic::fstok::CodeForKeyword;
using lwbasic::fstok::KeywordForCode;
using lwbasic::fstok::Tokens;

TEST_CASE("Token table: the anchors GW-BASIC 3.23 is known by", "[fstok]") {
  REQUIRE(CodeForKeyword("END").has_value());
  CHECK(*CodeForKeyword("END") == 0x81);
  CHECK(*CodeForKeyword("FOR") == 0x82);
  CHECK(*CodeForKeyword("DATA") == 0x84);
  CHECK(*CodeForKeyword("LET") == 0x88);
  CHECK(*CodeForKeyword("RUN") == 0x8A);
  CHECK(*CodeForKeyword("PRINT") == 0x91);
  CHECK(*CodeForKeyword("REM") == 0x8F);
  CHECK(*CodeForKeyword("ELSE") == 0xA1);
  CHECK(*CodeForKeyword("CLS") == 0xC0);
  CHECK(*CodeForKeyword("THEN") == 0xCD);
  CHECK(*CodeForKeyword("+") == 0xE9);
  CHECK(*CodeForKeyword("\\") == 0xF4);
  // Two-byte groups.
  CHECK(*CodeForKeyword("MKD$") == 0xFD86);
  CHECK(*CodeForKeyword("TIMER") == 0xFE94);
  CHECK(*CodeForKeyword("MKDIR") == 0xFE98);
  CHECK(*CodeForKeyword("EOF") == 0xFFA3);
  CHECK(*CodeForKeyword("LEFT$") == 0xFF81);
  CHECK(*CodeForKeyword("LOF") == 0xFFA5);
  CHECK_FALSE(CodeForKeyword("NOT_A_KEYWORD").has_value());
}

TEST_CASE("Token table: codes are unique and keywords are non-empty", "[fstok]") {
  std::set<std::uint16_t> codes;
  for (const auto& token : Tokens()) {
    INFO("code " << token.code << " keyword [" << token.keyword << "]");
    CHECK(codes.insert(token.code).second); // no duplicates
    CHECK(token.keyword != nullptr);
    CHECK(std::string(token.keyword).size() > 0);
  }
}

TEST_CASE("Token table: single-byte codes are ascending and inside their range", "[fstok]") {
  std::uint16_t previous = 0;
  for (const auto& token : Tokens()) {
    if (lwbasic::fstok::IsTwoByteToken(token.code))
      continue;
    INFO("code " << token.code);
    CHECK(token.code >= 0x81);
    CHECK(token.code <= 0xF4);
    CHECK(token.code > previous); // ascending, so lookups can binary-search
    previous = token.code;
  }
  // The gap bytes GW-BASIC leaves undefined are absent, which is what lets a
  // decoder treat them as invalid rather than guessing.
  for (unsigned int undefined :
       {0x9Au, 0x9Bu, 0x9Fu, 0xB4u, 0xB5u, 0xB6u, 0xCBu, 0xDFu, 0xE0u, 0xE5u, 0xF5u, 0xFCu}) {
    CHECK_FALSE(KeywordForCode(static_cast<std::uint16_t>(undefined)).has_value());
  }
}

TEST_CASE("Token table: every keyword maps back to its own code", "[fstok]") {
  for (const auto& token : Tokens()) {
    const std::string keyword = token.keyword;
    INFO("keyword [" << keyword << "]");
    const std::optional<std::uint16_t> code = CodeForKeyword(keyword);
    REQUIRE(code.has_value());
    CHECK(*code == token.code);
    const std::optional<std::string> back = KeywordForCode(token.code);
    REQUIRE(back.has_value());
    CHECK(*back == keyword);
  }
}

TEST_CASE("Token table: the two-byte prefixes are the documented ones", "[fstok]") {
  for (const auto& token : Tokens()) {
    if (!lwbasic::fstok::IsTwoByteToken(token.code))
      continue;
    const std::uint8_t prefix = lwbasic::fstok::TokenPrefix(token.code);
    INFO("keyword [" << token.keyword << "]");
    CHECK((prefix == 0xFD || prefix == 0xFE || prefix == 0xFF));
    CHECK(lwbasic::fstok::TokenSuffix(token.code) >= 0x81);
  }
}

TEST_CASE("Token table: numeric and operator token constants", "[fstok]") {
  CHECK(lwbasic::fstok::kTokenNumberUInt == 0x0E);
  CHECK(lwbasic::fstok::kTokenNumberOct == 0x0B);
  CHECK(lwbasic::fstok::kTokenNumberHex == 0x0C);
  CHECK(lwbasic::fstok::kTokenNumberInt == 0x1C);
  CHECK(lwbasic::fstok::kTokenNumberSingle == 0x1D);
  CHECK(lwbasic::fstok::kTokenNumberDouble == 0x1F);
  CHECK(lwbasic::fstok::kTokenConstantZero == 0x11);
}

TEST_CASE("MBF single: the classic byte patterns", "[fstok][mbf]") {
  std::uint8_t bytes[4] = {0, 0, 0, 0};
  // 1.0 = 1.0 x 2^0 -> exponent 0x80, zero fraction.
  lwbasic::fstok::EncodeMbfSingle(1.0, bytes);
  CHECK(bytes[0] == 0x80);
  CHECK(bytes[1] == 0x00);
  CHECK(bytes[2] == 0x00);
  CHECK(bytes[3] == 0x00);
  // 2.0 -> exponent 0x81; 0.5 -> exponent 0x7F.
  lwbasic::fstok::EncodeMbfSingle(2.0, bytes);
  CHECK(bytes[0] == 0x81);
  lwbasic::fstok::EncodeMbfSingle(0.5, bytes);
  CHECK(bytes[0] == 0x7F);
  // -1.0 sets the sign bit at the top of the first mantissa byte.
  lwbasic::fstok::EncodeMbfSingle(-1.0, bytes);
  CHECK(bytes[0] == 0x80);
  CHECK(bytes[1] == 0x80);
  // 1.5 -> fraction 0.5 -> 0x400000, so 0x40 in the mantissa's top byte.
  lwbasic::fstok::EncodeMbfSingle(1.5, bytes);
  CHECK(bytes[0] == 0x80);
  CHECK(bytes[1] == 0x40);
  CHECK(bytes[2] == 0x00);
  CHECK(bytes[3] == 0x00);
  // Zero is all zero bytes (MBF's only special case).
  lwbasic::fstok::EncodeMbfSingle(0.0, bytes);
  CHECK(bytes[0] == 0x00);
  CHECK(bytes[1] == 0x00);
  const std::uint8_t zero[4] = {0, 0, 0, 0};
  CHECK(lwbasic::fstok::DecodeMbfSingle(zero) == 0.0);
}

TEST_CASE("MBF single: round trips within its 23-bit precision", "[fstok][mbf]") {
  const double values[] = {1.0,  -1.0,   2.0,       0.5,    1.5,    3.14159265358979,
                           1e10, -1e-10, 12345.678, 0.0001, 6.02e23};
  for (double value : values) {
    std::uint8_t bytes[4] = {0, 0, 0, 0};
    lwbasic::fstok::EncodeMbfSingle(value, bytes);
    const double back = lwbasic::fstok::DecodeMbfSingle(bytes);
    INFO("value " << value << " back " << back);
    CHECK(back == Approx(value).epsilon(2e-7));
  }
}

TEST_CASE("MBF double: known patterns and round trips", "[fstok][mbf]") {
  std::uint8_t bytes[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  lwbasic::fstok::EncodeMbfDouble(1.0, bytes);
  CHECK(bytes[0] == 0x80);
  for (int i = 1; i < 8; ++i) {
    CHECK(bytes[i] == 0x00);
  }
  lwbasic::fstok::EncodeMbfDouble(-2.0, bytes);
  CHECK(bytes[0] == 0x81);
  CHECK(bytes[1] == 0x80);
  const std::uint8_t zero[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  CHECK(lwbasic::fstok::DecodeMbfDouble(zero) == 0.0);

  const double values[] = {1.0, -1.0, 0.5, 3.14159265358979, 1e30, -2.5e-30};
  for (double value : values) {
    lwbasic::fstok::EncodeMbfDouble(value, bytes);
    const double back = lwbasic::fstok::DecodeMbfDouble(bytes);
    INFO("value " << value << " back " << back);
    CHECK(back == Approx(value).epsilon(1e-14));
  }
}

TEST_CASE("MBF: the two encodings agree on shared values", "[fstok][mbf]") {
  // A single-precision encoding is the double encoding of a nearby value: both
  // must decode to the same number to the single's precision.
  for (double value : {0.0, 1.0, -3.5, 100.25, 1e5}) {
    std::uint8_t single[4] = {0, 0, 0, 0};
    lwbasic::fstok::EncodeMbfSingle(value, single);
    const double from_single = lwbasic::fstok::DecodeMbfSingle(single);
    std::uint8_t as_double[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    lwbasic::fstok::EncodeMbfDouble(from_single, as_double);
    CHECK(lwbasic::fstok::DecodeMbfDouble(as_double) == Approx(from_single).epsilon(1e-14));
  }
}
