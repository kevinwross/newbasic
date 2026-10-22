#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "newbasic/value.h"

namespace newbasic {

// Token kinds for one physical source line.
enum class Tok : std::uint8_t {
  EndOfLine, // end of the physical line
  Number,    // numeric literal (see Token::number)
  String,    // string literal (see Token::str)
  Ident,     // variable or user function name (upper-cased)
  Keyword,   // reserved word (upper-cased)
  Op,        // operator or punctuation (text is the operator: "+", "<=", ",", ...)
};

struct Token {
  Tok kind = Tok::EndOfLine;
  std::string text;          // Ident/Keyword/Op text, upper-cased
  Value number;              // Number
  std::string str;           // String bytes (CP437, unescaped)
  int pos = 0;               // 0-based column in the source line
  bool space_before = false; // whitespace preceded this token
};

// Lexer for the GW-BASIC dialect (plan section 2).
//
// Keywords are recognized only when the whole scanned name is reserved, so
// `TOTAL` is a name while `TO` is a keyword. Keyword and operand still need
// separation: `FOR I=1 TO 10` works; `FORI=1TO10` does not.
class Lexer {
public:
  // Tokenizes one physical line (no trailing newline). Always ends with an
  // EndOfLine token. Returns false and fills *error / *error_pos on a
  // malformed literal.
  static bool Tokenize(const std::string& line, std::vector<Token>* out, std::string* error,
                       int* error_pos);
  // True when `upper` is a reserved word.
  static bool IsKeyword(const std::string& upper);
  // Test seam: the reserved-word table, which IsKeyword binary-searches and
  // which must therefore stay strictly ascending with no duplicates.
  static std::size_t KeywordCount();
  static const char* KeywordAt(std::size_t index);
  // Uppercases an ASCII/CP437 name the way the dialect compares names.
  static std::string Upper(const std::string& text);
};

} // namespace newbasic
