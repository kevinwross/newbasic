#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/lexer.h"
#include "newbasic/value.h"

using newbasic::Lexer;
using newbasic::Tok;
using newbasic::Token;

namespace {

std::vector<Token> Lex(const std::string& line) {
  std::vector<Token> tokens;
  std::string error;
  int pos = 0;
  REQUIRE(Lexer::Tokenize(line, &tokens, &error, &pos));
  return tokens;
}

} // namespace

TEST_CASE("Lexer: keywords are whole words", "[lexer]") {
  const std::vector<Token> spaced = Lex("FOR I=1 TO 10");
  REQUIRE(spaced.size() == 7);
  CHECK(spaced[0].kind == Tok::Keyword);
  CHECK(spaced[0].text == "FOR");
  CHECK(spaced[1].kind == Tok::Ident);
  CHECK(spaced[1].text == "I");
  CHECK(spaced[4].kind == Tok::Keyword);
  CHECK(spaced[4].text == "TO");

  // Run-together keywords are names now; callers must write `FOR I = 1 TO 10`.
  const std::vector<Token> glued = Lex("FORI=1TO10");
  REQUIRE(glued.size() == 5);
  CHECK(glued[0].kind == Tok::Ident);
  CHECK(glued[0].text == "FORI");
  CHECK(glued[3].kind == Tok::Ident);
  CHECK(glued[3].text == "TO10");

  CHECK(Lex("TOTAL=1")[0].kind == Tok::Ident);
  CHECK(Lex("TOTAL=1")[0].text == "TOTAL");
  CHECK(Lex("PRINTX=1")[0].kind == Tok::Ident);
  CHECK(Lex("FNA(1)")[0].kind == Tok::Ident);
}

TEST_CASE("Lexer: identifiers keep type suffixes", "[lexer]") {
  const std::vector<Token> tokens = Lex("A$=B%+C#");
  REQUIRE(tokens.size() == 6);
  CHECK(tokens[0].text == "A$");
  CHECK(tokens[1].text == "=");
  CHECK(tokens[2].text == "B%");
  CHECK(tokens[3].text == "+");
  CHECK(tokens[4].text == "C#");
  const std::vector<Token> name = Lex("SCORE=1");
  CHECK(name[0].kind == Tok::Ident);
  CHECK(name[0].text == "SCORE");
  const std::vector<Token> prefixed = Lex("XPRINT=1");
  CHECK(prefixed[0].kind == Tok::Ident);
  CHECK(prefixed[0].text == "XPRINT");
}

TEST_CASE("Lexer: numeric literal kinds", "[lexer]") {
  CHECK(Lex("1")[0].number.kind() == newbasic::Value::Kind::Int);
  CHECK(Lex("32767")[0].number.kind() == newbasic::Value::Kind::Int);
  CHECK(Lex("40000")[0].number.kind() == newbasic::Value::Kind::Int);
  CHECK(Lex("1.5")[0].number.kind() == newbasic::Value::Kind::Sgl);
  CHECK(Lex("12345678")[0].number.kind() == newbasic::Value::Kind::Dbl);
  CHECK(Lex("1E3")[0].number.kind() == newbasic::Value::Kind::Sgl);
  CHECK(Lex("1D3")[0].number.kind() == newbasic::Value::Kind::Dbl);
  CHECK(Lex("5%")[0].number.kind() == newbasic::Value::Kind::Int);
  CHECK(Lex("40000%")[0].number.kind() == newbasic::Value::Kind::Int);
  CHECK(Lex("2147483647%")[0].number.AsNumber() == 2147483647.0);
  CHECK(Lex("5!")[0].number.kind() == newbasic::Value::Kind::Sgl);
  CHECK(Lex("5#")[0].number.kind() == newbasic::Value::Kind::Dbl);
  CHECK(Lex("&HFF")[0].number.kind() == newbasic::Value::Kind::Int);
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
