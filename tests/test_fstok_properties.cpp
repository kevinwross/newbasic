// M4: property coverage for the tokenized codec.
//
// The property is byte-level idempotence rather than text equality, because
// the number encodings are canonical: decoding re-formats the value as text
// (1.50 becomes 1.5), so the round trip is only required to be stable from the
// second pass on. That still catches structural bugs in the line records,
// offsets, token widths and terminators.
#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include <cstddef>
#include <string>
#include <vector>

#include "lwbasic/fstok/codec.h"
#include "lwbasic/fstok/encoder.h"

using lwbasic::fstok::DecodedLine;
using lwbasic::fstok::DecodeTokenized;
using lwbasic::fstok::EncodeTokenized;

namespace {

// A restricted alphabet keeps generated sources in the dialect's own spelling
// (keywords, operators, numbers) so the encoder exercises real token paths.
const std::vector<std::string>& Pieces() {
  static const std::vector<std::string> pieces = {
      "PRINT ", "LET ", "IF ", "THEN ", "GOTO ",  "FOR ",   "TO ", "NEXT ",  "REM ",
      "END",    "A",    "B$",  "=",     "+",      "-",      "*",   "/",      "^",
      ">",      "<",    "(",   ")",     ",",      ";",      ": ",  "\"HI\"", "0",
      "1",      "10",   "255", "4660",  "&H2D4F", "&O3627", "1.5", " "};
  return pieces;
}

std::string SourceFromPicks(const std::vector<int>& picks, std::size_t limit) {
  std::string source;
  for (std::size_t i = 0; i < picks.size() && i < limit; ++i) {
    const std::size_t index = static_cast<std::size_t>(picks[i]) % Pieces().size();
    source += Pieces()[index];
  }
  return source;
}

} // namespace

TEST_CASE("Property: tokenized encoding is idempotent under decode", "[fstok][property]") {
  rc::prop("encode(decode(encode(x))) == encode(x)", []() {
    const int count = *rc::gen::inRange(0, 4);
    std::vector<DecodedLine> lines;
    for (int i = 0; i < count; ++i) {
      const std::vector<int> picks = *rc::gen::container<std::vector<int>>(
          rc::gen::inRange(0, static_cast<int>(Pieces().size())));
      DecodedLine line;
      line.number = 1 + *rc::gen::inRange(0, 32000);
      line.source = SourceFromPicks(picks, 10);
      lines.push_back(std::move(line));
    }

    const std::string first = EncodeTokenized(lines);
    std::vector<DecodedLine> decoded;
    std::string error;
    RC_ASSERT(DecodeTokenized(first, &decoded, &error));
    RC_ASSERT(decoded.size() == lines.size());
    const std::string second = EncodeTokenized(decoded);
    RC_ASSERT(second == first);
  });
}

TEST_CASE("Property: every generated source survives print round trips", "[fstok][property]") {
  rc::prop("a PRINT of generated text encodes, decodes and re-encodes identically", []() {
    const std::vector<int> picks = *rc::gen::container<std::vector<int>>(
        rc::gen::inRange(0, static_cast<int>(Pieces().size())));
    std::vector<DecodedLine> lines = {{10, "PRINT \"" + SourceFromPicks(picks, 8) + "\""},
                                      {20, "END"}};
    const std::string first = EncodeTokenized(lines);
    std::vector<DecodedLine> decoded;
    std::string error;
    RC_ASSERT(DecodeTokenized(first, &decoded, &error));
    RC_ASSERT(decoded.size() == 2);
    RC_ASSERT(EncodeTokenized(decoded) == first);
  });
}

TEST_CASE("Property: decoding never returns more lines than were encoded", "[fstok][property]") {
  rc::prop("line count and numbers are preserved", []() {
    const int count = *rc::gen::inRange(0, 5);
    std::vector<DecodedLine> lines;
    for (int i = 0; i < count; ++i) {
      DecodedLine line;
      line.number = 1 + i;
      line.source = "END";
      lines.push_back(std::move(line));
    }
    const std::string bytes = EncodeTokenized(lines);
    std::vector<DecodedLine> decoded;
    std::string error;
    RC_ASSERT(DecodeTokenized(bytes, &decoded, &error));
    RC_ASSERT(decoded.size() == static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
      RC_ASSERT(decoded[static_cast<std::size_t>(i)].number == 1 + i);
    }
  });
}

// Regression: the shrunk counterexample from a property failure (seed
// 1702061189). It is byte-level, so it is pinned here as a plain test.
TEST_CASE("Tokenized: hex constant re-encodes to the same bytes", "[fstok][codec]") {
  const char raw[] = "\xFF\x6\0\x1\0\0\x13\0\x1\0\f\x10O\x1D\x7F\0\0\0\0\0\0\x1A";
  const std::string original(raw, sizeof(raw) - 1);
  std::vector<DecodedLine> decoded;
  std::string error;
  REQUIRE(DecodeTokenized(original, &decoded, &error));
  std::string joined;
  for (const DecodedLine& line : decoded) {
    joined += std::to_string(line.number) + ":[" + line.source + "] ";
  }
  INFO("decoded: " << joined);
  const std::string reencoded = EncodeTokenized(decoded);
  INFO("original  size " << original.size() << ", reencoded size " << reencoded.size());
  CHECK(reencoded == original);
}
