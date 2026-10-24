// Plain-text .BAS format (plan section 4): property coverage for the
// ToTextBas / ParseTextBas round trip plus byte-level format checks.
//
// The format is "<line number><tab><source>" per line with CRLF endings, so
// it cannot represent a source line whose content starts with whitespace
// (the parser consumes the separator run) or contains CR/LF; the property
// normalises those inputs instead of pretending otherwise.
#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include <string>
#include <utility>
#include <vector>

#include "newbasic/line_store.h"

namespace {

std::string NormaliseForTextFormat(const std::string& input) {
  std::string out;
  for (char c : input) {
    out += (c == '\r' || c == '\n' || c == '\t') ? ' ' : c;
  }
  std::size_t begin = 0;
  std::size_t end = out.size();
  while (begin < end && (out[begin] == ' ' || out[begin] == '\t'))
    ++begin;
  while (end > begin && (out[end - 1] == ' ' || out[end - 1] == '\t'))
    --end;
  return out.substr(begin, end - begin);
}

} // namespace

TEST_CASE("Text .BAS: byte-level format", "[bas]") {
  newbasic::LineStore store;
  store.Add(10, "PRINT \"HI\"");
  store.Add(20, "PRINT 42");
  CHECK(store.ToTextBas() == "10\tPRINT \"HI\"\r\n20\tPRINT 42\r\n");
}

TEST_CASE("Text .BAS: high CP437 bytes survive the round trip", "[bas]") {
  // 0xDB (block) and 0xE9 (theta-ish) are CP437 glyphs; they must come back
  // byte-identical because the format preserves raw bytes.
  const std::string source = "PRINT \"\xDB\xE9\"";
  newbasic::LineStore store;
  store.Add(10, source);
  const std::string text = store.ToTextBas();
  newbasic::LineStore reloaded;
  std::string error;
  REQUIRE(newbasic::LineStore::ParseTextBas(text, &reloaded, &error));
  REQUIRE(reloaded.Size() == 1);
  CHECK(reloaded.Lines()[0].source == source);
}

TEST_CASE("Text .BAS: LF input is accepted on load", "[bas]") {
  newbasic::LineStore store;
  std::string error;
  REQUIRE(newbasic::LineStore::ParseTextBas("10 PRINT 1\n20 PRINT 2\n", &store, &error));
  CHECK(store.Size() == 2);
  CHECK(store.Lines()[1].source == "PRINT 2");
  // Saving always emits CRLF.
  CHECK(store.ToTextBas() == "10\tPRINT 1\r\n20\tPRINT 2\r\n");
}

TEST_CASE("Property: text .BAS round-trips through save and load", "[property][bas]") {
  rc::prop("save then load reproduces the program exactly",
           [](const std::vector<std::pair<int, std::string>>& entries) {
             newbasic::LineStore store;
             for (const auto& entry : entries) {
               if (entry.first < 1 || entry.first > newbasic::kMaxLineNumber)
                 continue;
               const std::string source = NormaliseForTextFormat(entry.second);
               if (source.empty())
                 continue;
               store.Add(entry.first, source);
             }
             const std::string text = store.ToTextBas();
             newbasic::LineStore reloaded;
             std::string error;
             RC_ASSERT(newbasic::LineStore::ParseTextBas(text, &reloaded, &error));
             RC_ASSERT(reloaded.Size() == store.Size());
             for (std::size_t i = 0; i < store.Size(); ++i) {
               RC_ASSERT(reloaded.Lines()[i].line_number == store.Lines()[i].line_number);
               RC_ASSERT(reloaded.Lines()[i].source == store.Lines()[i].source);
             }
           });
}

TEST_CASE("Property: saved text is always ordered, tabbed and CRLF-terminated", "[property][bas]") {
  rc::prop("format invariants hold for any program",
           [](const std::vector<std::pair<int, std::string>>& entries) {
             newbasic::LineStore store;
             for (const auto& entry : entries) {
               if (entry.first < 1 || entry.first > newbasic::kMaxLineNumber)
                 continue;
               const std::string source = NormaliseForTextFormat(entry.second);
               if (source.empty())
                 continue;
               store.Add(entry.first, source);
             }
             const std::string text = store.ToTextBas();
             std::size_t line_count = 0;
             std::size_t position = 0;
             while (position < text.size()) {
               const std::size_t end = text.find("\r\n", position);
               RC_ASSERT(end != std::string::npos);
               const std::string record = text.substr(position, end - position);
               const std::size_t tab = record.find('\t');
               RC_ASSERT(tab != std::string::npos);
               RC_ASSERT(tab > 0);
               for (std::size_t i = 0; i < tab; ++i) {
                 RC_ASSERT(record[i] >= '0' && record[i] <= '9');
               }
               ++line_count;
               position = end + 2;
             }
             RC_ASSERT(line_count == store.Size());
           });
}
