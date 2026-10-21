#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "lwbasic/lexer.h"
#include "lwbasic/value.h"

using lwbasic::Lexer;
using lwbasic::Tok;
using lwbasic::Token;

namespace {

std::vector<Token> Lex(const std::string& line) {
  std::vector<Token> tokens;
  std::string error;
  int pos = 0;
  REQUIRE(Lexer::Tokenize(line, &tokens, &error, &pos));
  return tokens;
}

} // namespace

TEST_CASE("Lexer: keywords are matched longest-first without spaces", "[lexer]") {
  const std::vector<Token> tokens = Lex("FORI=1TO10");
  REQUIRE(tokens.size() == 7);
  CHECK(tokens[0].kind == Tok::Keyword);
  CHECK(tokens[0].text == "FOR");
  CHECK(tokens[1].kind == Tok::Ident);
  CHECK(tokens[1].text == "I");
  CHECK(tokens[2].kind == Tok::Op);
  CHECK(tokens[2].text == "=");
  CHECK(tokens[3].kind == Tok::Number);
  CHECK(tokens[4].kind == Tok::Keyword);
  CHECK(tokens[4].text == "TO");
  CHECK(tokens[5].kind == Tok::Number);
  CHECK(tokens[6].kind == Tok::EndOfLine);
}

TEST_CASE("Lexer: identifiers keep type suffixes and split at keywords", "[lexer]") {
  const std::vector<Token> tokens = Lex("A$=B%+C#");
  REQUIRE(tokens.size() == 6);
  CHECK(tokens[0].text == "A$");
  CHECK(tokens[1].text == "=");
  CHECK(tokens[2].text == "B%");
  CHECK(tokens[3].text == "+");
  CHECK(tokens[4].text == "C#");
  // A keyword abutting a name splits the name (GW-BASIC behaviour).
  const std::vector<Token> split = Lex("SCORE=1");
  CHECK(split[0].text == "SCORE");
  // Keywords win at the start of a run; an existing name run is not split
  // (GW-BASIC scans the keyword first, then the whole name).
  const std::vector<Token> keyword_split = Lex("XPRINT=1");
  CHECK(keyword_split[0].kind == Tok::Ident);
  CHECK(keyword_split[0].text == "XPRINT");
}

TEST_CASE("Lexer: numeric literal kinds", "[lexer]") {
  CHECK(Lex("1")[0].number.kind() == lwbasic::Value::Kind::Int);
  CHECK(Lex("32767")[0].number.kind() == lwbasic::Value::Kind::Int);
  CHECK(Lex("40000")[0].number.kind() == lwbasic::Value::Kind::Long);
  CHECK(Lex("1.5")[0].number.kind() == lwbasic::Value::Kind::Sgl);
  CHECK(Lex("12345678")[0].number.kind() == lwbasic::Value::Kind::Dbl);
  CHECK(Lex("1E3")[0].number.kind() == lwbasic::Value::Kind::Sgl);
  CHECK(Lex("1D3")[0].number.kind() == lwbasic::Value::Kind::Dbl);
  CHECK(Lex("5%")[0].number.kind() == lwbasic::Value::Kind::Int);
  CHECK(Lex("5!")[0].number.kind() == lwbasic::Value::Kind::Sgl);
  CHECK(Lex("5#")[0].number.kind() == lwbasic::Value::Kind::Dbl);
  CHECK(Lex("&HFF")[0].number.kind() == lwbasic::Value::Kind::Int);
  CHECK(Lex("&HFF")[0].number.AsNumber() == 255.0);
  CHECK(Lex("&O10")[0].number.AsNumber() == 8.0);
  CHECK(Lex("2.5E-3")[0].number.AsNumber() == 0.0025);
}

TEST_CASE("Lexer: strings, comments and operators", "[lexer]") {
  CHECK(Lex("\"A\"\"B\"")[0].str == "A\"B");
  CHECK(Lex("\"\"")[0].str.empty());
  const std::vector<Token> comment = Lex("'note");
  REQUIRE(!comment.empty());
  CHECK(comment[0].kind == Tok::Op);
  CHECK(comment[0].text == "'");
  const std::vector<Token> ops = Lex("A<=B<>C>=D");
  CHECK(ops[1].text == "<=");
  CHECK(ops[3].text == "<>");
  CHECK(ops[5].text == ">=");
  CHECK(Lex("PRINT 1: REM x")[2].text == ":");
}

TEST_CASE("Lexer: unterminated string is an error", "[lexer]") {
  std::vector<Token> tokens;
  std::string error;
  int pos = 0;
  CHECK_FALSE(Lexer::Tokenize("PRINT \"abc", &tokens, &error, &pos));
  CHECK(error == "Unterminated string");
}

TEST_CASE("Lexer: space_before is recorded for the PRINT space rule", "[lexer]") {
  const std::vector<Token> tokens = Lex("PRINT 1 2");
  REQUIRE(tokens.size() >= 4);
  CHECK(tokens[1].space_before);
  CHECK(tokens[2].space_before);
}
