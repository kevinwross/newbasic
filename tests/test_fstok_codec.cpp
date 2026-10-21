// M4: tokenized-BASIC decoder.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "lwbasic/fstok/codec.h"
#include "lwbasic/fstok/encoder.h"
#include "lwbasic/fstok/protect.h"

using lwbasic::fstok::DecodedLine;
using lwbasic::fstok::DecodeTokenized;

namespace {

// Builds a one-line file: envelope, next offset, line number, text, 0x00,
// end-of-program 0x0000, trailing 0x1A.
std::string OneLineFile(int number, const std::string& text) {
  const unsigned next_offset = static_cast<unsigned>(1 + 2 + 2 + text.size() + 1);
  std::string bytes;
  bytes += static_cast<char>(0xFF);
  bytes += static_cast<char>(next_offset & 0xFF);
  bytes += static_cast<char>((next_offset >> 8) & 0xFF);
  bytes += static_cast<char>(number & 0xFF);
  bytes += static_cast<char>((number >> 8) & 0xFF);
  bytes += text;
  bytes += static_cast<char>(0x00);
  bytes += static_cast<char>(0x00);
  bytes += static_cast<char>(0x00);
  bytes += static_cast<char>(0x1A);
  return bytes;
}

std::string DecodeError(const std::string& bytes) {
  std::vector<DecodedLine> lines;
  std::string error;
  CHECK_FALSE(DecodeTokenized(bytes, &lines, &error));
  return error;
}

} // namespace

TEST_CASE("Tokenized decode: one line of keywords and text", "[fstok][codec]") {
  // 0x91 PRINT, " ", '"', H, I, '"'
  const std::string text = std::string("\x91") + " \"HI\"";
  const std::string bytes = OneLineFile(10, text);
  std::vector<DecodedLine> lines;
  std::string error;
  REQUIRE(DecodeTokenized(bytes, &lines, &error));
  INFO("error: " << error);
  REQUIRE(lines.size() == 1);
  CHECK(lines[0].number == 10);
  CHECK(lines[0].source == "PRINT \"HI\"");
}

TEST_CASE("Tokenized decode: several lines keep their order and numbers", "[fstok][codec]") {
  // Two line records in one file.
  // The space after a keyword is stored as 0x20 in a real file, so the fixture
  // carries it too (the token replaces only the keyword text).
  const std::string first = std::string("\x88") + " A=1"; // LET A=1
  const std::string second = std::string("\x91") + " A";  // PRINT A
  const unsigned first_offset = static_cast<unsigned>(1 + 2 + 2 + first.size() + 1);
  const unsigned second_offset = first_offset + static_cast<unsigned>(2 + 2 + second.size() + 1);
  std::string bytes;
  bytes += static_cast<char>(0xFF);
  bytes += static_cast<char>(first_offset & 0xFF);
  bytes += static_cast<char>((first_offset >> 8) & 0xFF);
  bytes += static_cast<char>(10);
  bytes += static_cast<char>(0);
  bytes += first;
  bytes += static_cast<char>(0x00);
  bytes += static_cast<char>(second_offset & 0xFF);
  bytes += static_cast<char>((second_offset >> 8) & 0xFF);
  bytes += static_cast<char>(20);
  bytes += static_cast<char>(0);
  bytes += second;
  bytes += static_cast<char>(0x00);
  bytes += std::string("\x00\x00\x1A", 3);

  std::vector<DecodedLine> lines;
  std::string error;
  REQUIRE(DecodeTokenized(bytes, &lines, &error));
  REQUIRE(lines.size() == 2);
  CHECK(lines[0].number == 10);
  CHECK(lines[0].source == "LET A=1");
  CHECK(lines[1].number == 20);
  CHECK(lines[1].source == "PRINT A");
}

TEST_CASE("Tokenized decode: two-byte tokens and numeric constants", "[fstok][codec]") {
  // A=MKDIR 0xFE98 is a statement, so use a function: EOF = 0xFFA3, plus the
  // constants 0x11 (zero), 0x1C 0x34 0x12 (4660) and 0x0C 0x2D4F (hex 2D4F).
  std::string text = "A=";
  text += static_cast<char>(0xFF);
  text += static_cast<char>(0xA3); // EOF
  text += static_cast<char>(0x11); // constant 0
  text += static_cast<char>(0x1C);
  text += static_cast<char>(0x34);
  text += static_cast<char>(0x12); // 4660
  text += static_cast<char>(0x0C);
  text += static_cast<char>(0x4F);
  text += static_cast<char>(0x2D); // &H2D4F
  const std::string bytes = OneLineFile(30, text);
  std::vector<DecodedLine> lines;
  std::string error;
  REQUIRE(DecodeTokenized(bytes, &lines, &error));
  INFO("error: " << error);
  REQUIRE(lines.size() == 1);
  CHECK(lines[0].source == "A=EOF04660&H2D4F");
}

TEST_CASE("Tokenized decode: MBF constants become decimal text", "[fstok][codec]") {
  // 1.5 encodes as 80 40 00 00 (see the MBF tests).
  std::string text = "A=";
  text += static_cast<char>(0x1D);
  text += static_cast<char>(0x80);
  text += static_cast<char>(0x40);
  text += static_cast<char>(0x00);
  text += static_cast<char>(0x00);
  const std::string bytes = OneLineFile(40, text);
  std::vector<DecodedLine> lines;
  std::string error;
  REQUIRE(DecodeTokenized(bytes, &lines, &error));
  REQUIRE(lines.size() == 1);
  CHECK(lines[0].source == "A=1.5");
}

TEST_CASE("Tokenized decode: malformed input is reported, never guessed", "[fstok][codec]") {
  CHECK(DecodeError("") == "empty file");
  CHECK(DecodeError(std::string("\xFE\x00\x00", 3)) == "protected program");
  CHECK(DecodeError(std::string("\x00\x00\x00", 3)).find("not a tokenized") != std::string::npos);
  // Truncated: an offset that promises a line but no line number follows.
  CHECK(DecodeError(std::string("\xFF\x05\x00", 3)) == "truncated line number");
  // A line with no 0x00 terminator before the end of the data.
  std::string unterminated = std::string("\xFF\x07\x00\x0A\x00", 5);
  unterminated += "\x91"; // PRINT, never terminated
  CHECK(DecodeError(unterminated) == "unterminated line");
  // Undefined token 0x9A must be rejected, not skipped.
  std::string undefined = OneLineFile(10, std::string("\x9A"));
  CHECK(DecodeError(undefined).find("undefined token") != std::string::npos);
  // Truncated MBF constant.
  std::string short_single = std::string("\xFF\x08\x00\x0A\x00\x1D\x80", 7);
  CHECK(DecodeError(short_single).find("truncated single") != std::string::npos);
}

TEST_CASE("Protected files: the byte transform is its own inverse", "[fstok][protect]") {
  // The strongest check available without a real Microsoft-authored file: the
  // two directions must undo each other for every position in the 143-byte
  // period, for a spread of byte values.
  for (int index = 0; index < 143; ++index) {
    for (int value = 0; value < 256; value += 37) {
      const std::uint8_t original = static_cast<std::uint8_t>(value);
      const std::uint8_t protected_byte = lwbasic::fstok::ProtectByte(original, index);
      const std::uint8_t restored = lwbasic::fstok::UnprotectByte(protected_byte, index);
      INFO("index " << index << " value " << value);
      CHECK(restored == original);
    }
  }
}

TEST_CASE("Protected files: envelope and round trip through the codec", "[fstok][protect]") {
  const std::vector<DecodedLine> lines = {
      {10, "PRINT \"SECRET\""}, {20, "LET A=1"}, {30, "A=1.5"}, {40, "END"}};
  const std::string tokenized = lwbasic::fstok::EncodeTokenized(lines);
  REQUIRE(static_cast<unsigned char>(tokenized[0]) == 0xFF);

  const std::string protected_bytes = lwbasic::fstok::ProtectTokenized(tokenized);
  REQUIRE(protected_bytes.size() == tokenized.size());
  CHECK(static_cast<unsigned char>(protected_bytes[0]) == 0xFE);
  // Everything after the envelope must actually change...
  CHECK(protected_bytes != tokenized);
  // ...while the trailing EOF marker is preserved as the description says.
  CHECK(protected_bytes.back() == tokenized.back());

  // Decoding the protected form directly is what the decoder rejects, so a
  // program has to be unprotected first.
  std::vector<DecodedLine> rejected;
  std::string rejection;
  CHECK_FALSE(lwbasic::fstok::DecodeTokenized(protected_bytes, &rejected, &rejection));
  CHECK(rejection == "protected program");

  const std::string restored = lwbasic::fstok::UnprotectTokenized(protected_bytes);
  CHECK(restored == tokenized);
  std::vector<DecodedLine> decoded;
  std::string error;
  REQUIRE(lwbasic::fstok::DecodeTokenized(restored, &decoded, &error));
  REQUIRE(decoded.size() == lines.size());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    CHECK(decoded[i].number == lines[i].number);
    CHECK(decoded[i].source == lines[i].source);
  }
}

TEST_CASE("Tokenized encode: envelope, offsets and trailer", "[fstok][codec]") {
  const std::vector<DecodedLine> lines = {{10, "PRINT \"HI\""}, {20, "END"}};
  const std::string bytes = lwbasic::fstok::EncodeTokenized(lines);
  REQUIRE(bytes.size() > 4);
  CHECK(static_cast<unsigned char>(bytes[0]) == 0xFF);
  CHECK(static_cast<unsigned char>(bytes[bytes.size() - 1]) == 0x1A);
  // First record: offset of the next record, then the line number.
  const unsigned first_offset = static_cast<unsigned>(static_cast<unsigned char>(bytes[1])) |
                                (static_cast<unsigned>(static_cast<unsigned char>(bytes[2])) << 8);
  CHECK(first_offset == 1 + 2 + 2 + std::string("\x91 \"HI\"").size() + 1);
  CHECK(static_cast<unsigned char>(bytes[3]) == 10);
  CHECK(static_cast<unsigned char>(bytes[4]) == 0);
  // PRINT then the text with its space kept.
  CHECK(static_cast<unsigned char>(bytes[5]) == 0x91);
  CHECK(bytes.compare(6, 5, " \"HI\"") == 0);
}

TEST_CASE("Tokenized: encode then decode round-trips", "[fstok][codec]") {
  const std::vector<DecodedLine> lines = {
      {10, "PRINT \"HI\""},
      {20, "LET A=1"},
      {30, "A=&H2D4F"},
      {40, "B=1.5"},
      {50, "C=0"},
      {60, "D=4660"},
      {70, "IF A>1 THEN GOTO 10"},
      {80, "FOR I=1 TO 10: NEXT I"},
      {90, "REM hello there"},
      {100, "END"},
  };
  const std::string bytes = lwbasic::fstok::EncodeTokenized(lines);
  std::vector<DecodedLine> decoded;
  std::string error;
  REQUIRE(lwbasic::fstok::DecodeTokenized(bytes, &decoded, &error));
  INFO("decode error: " << error);
  REQUIRE(decoded.size() == lines.size());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    INFO("line " << i << ": [" << lines[i].source << "] vs [" << decoded[i].source << "]");
    CHECK(decoded[i].number == lines[i].number);
    CHECK(decoded[i].source == lines[i].source);
  }
}

TEST_CASE("Tokenized: a second encoding of the decoded text is byte-identical", "[fstok][codec]") {
  const std::vector<DecodedLine> lines = {{10, "PRINT 42"}, {20, "END"}};
  const std::string first = lwbasic::fstok::EncodeTokenized(lines);
  std::vector<DecodedLine> decoded;
  std::string error;
  REQUIRE(lwbasic::fstok::DecodeTokenized(first, &decoded, &error));
  CHECK(lwbasic::fstok::EncodeTokenized(decoded) == first);
}

TEST_CASE("Tokenized: operators and comparison tokens survive the round trip", "[fstok][codec]") {
  const std::vector<DecodedLine> lines = {
      {10, "A=1+2*3-4/5"},        {20, "B=A^2"},  {30, "C=A>=1"}, {40, "D=A<=1"}, {50, "E=A<>1"},
      {60, "F=A AND B OR NOT C"}, {70, "G=A\\2"},
  };
  const std::string bytes = lwbasic::fstok::EncodeTokenized(lines);
  std::vector<DecodedLine> decoded;
  std::string error;
  REQUIRE(lwbasic::fstok::DecodeTokenized(bytes, &decoded, &error));
  REQUIRE(decoded.size() == lines.size());
  for (std::size_t i = 0; i < lines.size(); ++i) {
    INFO("line [" << lines[i].source << "] vs [" << decoded[i].source << "]");
    CHECK(decoded[i].source == lines[i].source);
  }
}
