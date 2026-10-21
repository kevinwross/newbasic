// Robustness coverage: keyword-table completeness and crash-safety of the
// front end on arbitrary input.
//
// The keyword table has been the source of three separate regressions (CALL
// and CLS dropped from a packed table edit, then MKDIR silently missing), so
// the words the compiler dispatches on are asserted here explicitly.
#include <catch2/catch_test_macros.hpp>

#include <cctype>
#include <random>
#include <string>

#include "newbasic/errors.h"
#include "newbasic/lexer.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

TEST_CASE("Lexer: the reserved-word table is sorted and duplicate-free", "[lexer][robust]") {
  // IsKeyword binary-searches this table, so a mis-sorted or duplicated entry
  // makes lookups silently fail. The packed table has lost entries three
  // times; this checks the whole table rather than known words only.
  const std::size_t count = newbasic::Lexer::KeywordCount();
  REQUIRE(count > 100);
  for (std::size_t i = 0; i < count; ++i) {
    const char* raw = newbasic::Lexer::KeywordAt(i);
    REQUIRE(raw != nullptr);
    const std::string word = raw;
    INFO("index " << i << ": [" << word << "]");
    CHECK_FALSE(word.empty());
    for (char c : word) {
      CHECK(c == std::toupper(static_cast<unsigned char>(c)));
    }
    if (i > 0) {
      const std::string previous = newbasic::Lexer::KeywordAt(i - 1);
      CHECK(previous < word); // strictly ascending: sorted, no duplicates
    }
  }
}

TEST_CASE("Lexer: statement keywords the compiler relies on are reserved", "[lexer][robust]") {
  static const char* const kDispatchWords[] = {
      // statements
      "PRINT",
      "INPUT",
      "LINE",
      "IF",
      "THEN",
      "ELSE",
      "FOR",
      "TO",
      "STEP",
      "NEXT",
      "WHILE",
      "WEND",
      "GOTO",
      "GOSUB",
      "RETURN",
      "ON",
      "END",
      "STOP",
      "REM",
      "LET",
      "CLS",
      "LOCATE",
      "COLOR",
      "RANDOMIZE",
      "SWAP",
      "BEEP",
      "SYSTEM",
      "DIM",
      "ERASE",
      "OPTION",
      "BASE",
      "DEF",
      "DEFINT",
      "DEFSNG",
      "DEFDBL",
      "DEFSTR",
      "FN",
      "DATA",
      "READ",
      "RESTORE",
      "RESUME",
      "ERROR",
      "SHELL",
      "OPEN",
      "CLOSE",
      "WRITE",
      "TIMER",
      "RUN",
      "LIST",
      "NEW",
      "CLEAR",
      "DELETE",
      "LOAD",
      "SAVE",
      "FILES",
      "KILL",
      "NAME",
      "MKDIR",
      "CHDIR",
      "RMDIR",
      // operators
      "AND",
      "OR",
      "NOT",
      "XOR",
      "EQV",
      "IMP",
      "MOD",
      // functions
      "ABS",
      "ASC",
      "ATN",
      "CDBL",
      "CHR$",
      "CINT",
      "COS",
      "CSNG",
      "CSRLIN",
      "DATE$",
      "EOF",
      "ERR",
      "ERL",
      "EXP",
      "FIX",
      "HEX$",
      "INKEY$",
      "INSTR",
      "INT",
      "LEFT$",
      "LEN",
      "LOG",
      "MID$",
      "OCT$",
      "POS",
      "RIGHT$",
      "RND",
      "SGN",
      "SIN",
      "SPACE$",
      "SQR",
      "STR$",
      "STRING$",
      "TAN",
      "TIME$",
      "VAL",
  };
  for (const char* word : kDispatchWords) {
    INFO("keyword: " << word);
    CHECK(newbasic::Lexer::IsKeyword(word));
  }
}

TEST_CASE("Lexer: keyword runs split the way GW-BASIC tokenizes them", "[lexer][robust]") {
  // No-space forms must still lex as keyword + operand.
  const std::vector<newbasic::Token> fori = lwtest::TokenizeLine("FORI=1TO10");
  REQUIRE(fori.size() >= 6);
  CHECK(fori[0].text == "FOR");
  CHECK(fori[1].text == "I");
  CHECK(fori[4].text == "TO");

  // OUTPUT lexes as OUT + PUT (the OPEN parser accepts the split).
  const std::vector<newbasic::Token> output = lwtest::TokenizeLine("OUTPUT AS #1");
  REQUIRE(output.size() >= 2);
  CHECK(output[0].text == "OUT");
  CHECK(output[1].text == "PUT");

  // A name that starts with FN is the DEF FN form.
  const std::vector<newbasic::Token> fn = lwtest::TokenizeLine("FNA(1)");
  REQUIRE(fn.size() >= 2);
  CHECK(fn[0].text == "FN");
  CHECK(fn[1].text == "A");
}

TEST_CASE("Front end: arbitrary input never crashes or hangs", "[robust]") {
  std::mt19937 rng(20261016u);
  const std::string alphabet = "ABCXYZ0123456789 <>+-*/\\^=(),;:\"$#%&!?'.";
  for (int iteration = 0; iteration < 400; ++iteration) {
    std::string line;
    const std::size_t length = static_cast<std::size_t>(rng() % 24u);
    for (std::size_t i = 0; i < length; ++i) {
      line += alphabet[static_cast<std::size_t>(rng() % alphabet.size())];
    }
    INFO("input: [" << line << "]");
    newbasic::screen::HeadlessScreen screen;
    newbasic::ScriptedHost host;
    newbasic::Repl repl(screen, &host);
    // Reaching the end of the iteration without a crash, hang or assertion
    // failure is the property under test.
    repl.Process(line);
    CHECK(true);
  }
}

TEST_CASE("Front end: numeric and string edge cases do not crash", "[robust]") {
  // Deeply nested parentheses, empty literals, huge exponents, unclosed
  // constructs: all must come back as clean compile errors or runs.
  const char* const kCases[] = {
      "10 PRINT ((((((((((1))))))))))",
      "10 PRINT \"\"",
      "10 PRINT \"\"\"\"",
      "10 PRINT 1E308",
      "10 PRINT 1E-308",
      "10 PRINT ((((((((((1))))))))",
      "10 PRINT &H7FFFFFFF",
      "10 PRINT &O777777777",
      "10 PRINT 1/0",
      "10 PRINT \"a\"+\"b\"+\"c\"",
      "10 A(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)=1",
      "10 DEF FNX(X)=X: PRINT FNX(FNX(FNX(1)))",
      "10 IF 1 THEN IF 1 THEN IF 1 THEN PRINT \"OK\"",
      "10 FOR I=1 TO 2: FOR J=1 TO 2: NEXT J: NEXT I",
  };
  for (const char* source : kCases) {
    INFO("source: " << source);
    int line = 0;
    lwtest::RunErrorCode(std::string(source) + "\n", &line);
    CHECK(true);
  }
}
