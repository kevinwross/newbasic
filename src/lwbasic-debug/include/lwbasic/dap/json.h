#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace lwbasic::dap {

// Minimal JSON value for Debug Adapter Protocol messages: exactly what the
// protocol needs (objects, arrays, strings, numbers, booleans, null) with no
// external dependency.
//
// Objects keep insertion order and are stored as a vector of pairs rather
// than a map, so responses read in the order they were built (stable diffs)
// and the type stays complete while nesting, which a std::map member with an
// incomplete value type would not guarantee.
class Json {
public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  Json() = default;

  static Json Bool(bool value) {
    Json json;
    json.type_ = Type::Bool;
    json.bool_ = value;
    return json;
  }
  static Json Number(double value) {
    Json json;
    json.type_ = Type::Number;
    json.number_ = value;
    return json;
  }
  static Json Str(std::string value) {
    Json json;
    json.type_ = Type::String;
    json.string_ = std::move(value);
    return json;
  }
  static Json Array() {
    Json json;
    json.type_ = Type::Array;
    return json;
  }
  static Json Object() {
    Json json;
    json.type_ = Type::Object;
    return json;
  }

  Type type() const { return type_; }
  bool IsNull() const { return type_ == Type::Null; }
  bool IsBool() const { return type_ == Type::Bool; }
  bool IsNumber() const { return type_ == Type::Number; }
  bool IsString() const { return type_ == Type::String; }
  bool IsArray() const { return type_ == Type::Array; }
  bool IsObject() const { return type_ == Type::Object; }

  bool AsBool() const { return bool_; }
  double AsNumber() const { return number_; }
  const std::string& AsString() const { return string_; }

  void Set(const std::string& key, Json value) {
    for (auto& member : members_) {
      if (member.first == key) {
        member.second = std::move(value);
        return;
      }
    }
    members_.emplace_back(key, std::move(value));
  }
  const Json* Find(const std::string& key) const {
    for (const auto& member : members_) {
      if (member.first == key)
        return &member.second;
    }
    return nullptr;
  }
  const std::vector<std::pair<std::string, Json>>& Members() const { return members_; }

  void Push(Json value) { items_.push_back(std::move(value)); }
  const std::vector<Json>& Items() const { return items_; }
  std::size_t Size() const { return items_.size(); }

  std::string Dump() const {
    std::string out;
    DumpTo(&out);
    return out;
  }

  // Parses one complete JSON value. On failure returns false and sets `error`
  // to a short reason; `out` is left unspecified.
  static bool Parse(const std::string& text, Json* out, std::string* error) {
    Parser parser(text);
    if (!parser.ParseValue(out)) {
      if (error != nullptr)
        *error = parser.error();
      return false;
    }
    parser.SkipSpace();
    if (!parser.AtEnd()) {
      if (error != nullptr)
        *error = "trailing characters";
      return false;
    }
    return true;
  }

private:
  class Parser {
  public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool AtEnd() const { return pos_ >= text_.size(); }
    const std::string& error() const { return error_; }

    void SkipSpace() {
      while (pos_ < text_.size()) {
        const char c = text_[pos_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
          ++pos_;
        } else {
          break;
        }
      }
    }

    bool Fail(const char* reason) {
      if (error_.empty())
        error_ = reason;
      return false;
    }

    bool ParseValue(Json* out) {
      SkipSpace();
      if (AtEnd())
        return Fail("unexpected end of input");
      switch (text_[pos_]) {
      case '{':
        return ParseObject(out);
      case '[':
        return ParseArray(out);
      case '"': {
        std::string value;
        if (!ParseString(&value))
          return false;
        *out = Json::Str(std::move(value));
        return true;
      }
      case 't':
        return ParseLiteral("true", Json::Bool(true), out);
      case 'f':
        return ParseLiteral("false", Json::Bool(false), out);
      case 'n':
        return ParseLiteral("null", Json(), out);
      default:
        if (text_[pos_] != '-' && (text_[pos_] < '0' || text_[pos_] > '9')) {
          return Fail("unexpected character");
        }
        return ParseNumber(out);
      }
    }

  private:
    bool ParseLiteral(const char* literal, Json value, Json* out) {
      const std::size_t length = std::char_traits<char>::length(literal);
      if (text_.compare(pos_, length, literal) != 0)
        return Fail("invalid literal");
      pos_ += length;
      *out = std::move(value);
      return true;
    }

    bool ParseNumber(Json* out) {
      const char* begin = text_.c_str() + pos_;
      char* end = nullptr;
      const double value = std::strtod(begin, &end);
      if (end == begin)
        return Fail("invalid number");
      pos_ += static_cast<std::size_t>(end - begin);
      *out = Json::Number(value);
      return true;
    }

    bool ParseString(std::string* out) {
      if (text_[pos_] != '"')
        return Fail("expected a string");
      ++pos_;
      out->clear();
      while (true) {
        if (AtEnd())
          return Fail("unterminated string");
        const unsigned char c = static_cast<unsigned char>(text_[pos_]);
        if (c == '"') {
          ++pos_;
          return true;
        }
        if (c != '\\') {
          if (c < 0x20)
            return Fail("control character in string");
          out->push_back(static_cast<char>(c));
          ++pos_;
          continue;
        }
        ++pos_;
        if (AtEnd())
          return Fail("unterminated escape");
        const char escape = text_[pos_++];
        switch (escape) {
        case '"':
          out->push_back('"');
          break;
        case '\\':
          out->push_back('\\');
          break;
        case '/':
          out->push_back('/');
          break;
        case 'b':
          out->push_back('\b');
          break;
        case 'f':
          out->push_back('\f');
          break;
        case 'n':
          out->push_back('\n');
          break;
        case 'r':
          out->push_back('\r');
          break;
        case 't':
          out->push_back('\t');
          break;
        case 'u': {
          unsigned int code = 0;
          if (!ParseHex4(&code))
            return false;
          AppendUtf8(code, out);
          break;
        }
        default:
          return Fail("invalid escape");
        }
      }
    }

    bool ParseHex4(unsigned int* out) {
      if (pos_ + 4 > text_.size())
        return Fail("short \\u escape");
      unsigned int value = 0;
      for (int i = 0; i < 4; ++i) {
        const char c = text_[pos_++];
        unsigned int digit = 0;
        if (c >= '0' && c <= '9') {
          digit = static_cast<unsigned int>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
          digit = static_cast<unsigned int>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
          digit = static_cast<unsigned int>(c - 'A' + 10);
        } else {
          return Fail("invalid \\u escape");
        }
        value = value * 16 + digit;
      }
      *out = value;
      return true;
    }

    static void AppendUtf8(unsigned int code, std::string* out) {
      // BMP only; surrogate pairs are not combined (DAP payloads are UTF-8).
      if (code < 0x80) {
        out->push_back(static_cast<char>(code));
      } else if (code < 0x800) {
        out->push_back(static_cast<char>(0xC0 | (code >> 6)));
        out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
      } else {
        out->push_back(static_cast<char>(0xE0 | (code >> 12)));
        out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
      }
    }

    bool ParseArray(Json* out) {
      ++pos_; // '['
      Json array = Json::Array();
      SkipSpace();
      if (!AtEnd() && text_[pos_] == ']') {
        ++pos_;
        *out = std::move(array);
        return true;
      }
      while (true) {
        Json item;
        if (!ParseValue(&item))
          return false;
        array.Push(std::move(item));
        SkipSpace();
        if (AtEnd())
          return Fail("unterminated array");
        if (text_[pos_] == ',') {
          ++pos_;
          continue;
        }
        if (text_[pos_] == ']') {
          ++pos_;
          *out = std::move(array);
          return true;
        }
        return Fail("expected ',' or ']'");
      }
    }

    bool ParseObject(Json* out) {
      ++pos_; // '{'
      Json object = Json::Object();
      SkipSpace();
      if (!AtEnd() && text_[pos_] == '}') {
        ++pos_;
        *out = std::move(object);
        return true;
      }
      while (true) {
        SkipSpace();
        if (AtEnd() || text_[pos_] != '"')
          return Fail("expected a member name");
        std::string key;
        if (!ParseString(&key))
          return false;
        SkipSpace();
        if (AtEnd() || text_[pos_] != ':')
          return Fail("expected ':'");
        ++pos_;
        Json value;
        if (!ParseValue(&value))
          return false;
        object.Set(key, std::move(value));
        SkipSpace();
        if (AtEnd())
          return Fail("unterminated object");
        if (text_[pos_] == ',') {
          ++pos_;
          continue;
        }
        if (text_[pos_] == '}') {
          ++pos_;
          *out = std::move(object);
          return true;
        }
        return Fail("expected ',' or '}'");
      }
    }

    const std::string& text_;
    std::size_t pos_ = 0;
    std::string error_;
  };

  void DumpTo(std::string* out) const {
    switch (type_) {
    case Type::Null:
      *out += "null";
      return;
    case Type::Bool:
      *out += bool_ ? "true" : "false";
      return;
    case Type::Number: {
      const double rounded = static_cast<double>(static_cast<long long>(number_));
      if (number_ == rounded) {
        *out += std::to_string(static_cast<long long>(number_));
      } else {
        char buffer[40];
        std::snprintf(buffer, sizeof(buffer), "%.17g", number_);
        *out += buffer;
      }
      return;
    }
    case Type::String:
      DumpString(string_, out);
      return;
    case Type::Array: {
      *out += '[';
      for (std::size_t i = 0; i < items_.size(); ++i) {
        if (i != 0)
          *out += ',';
        items_[i].DumpTo(out);
      }
      *out += ']';
      return;
    }
    case Type::Object: {
      *out += '{';
      for (std::size_t i = 0; i < members_.size(); ++i) {
        if (i != 0)
          *out += ',';
        DumpString(members_[i].first, out);
        *out += ':';
        members_[i].second.DumpTo(out);
      }
      *out += '}';
      return;
    }
    }
  }

  static void DumpString(const std::string& value, std::string* out) {
    *out += '"';
    for (char raw : value) {
      const unsigned char c = static_cast<unsigned char>(raw);
      switch (c) {
      case '"':
        *out += "\\\"";
        break;
      case '\\':
        *out += "\\\\";
        break;
      case '\n':
        *out += "\\n";
        break;
      case '\r':
        *out += "\\r";
        break;
      case '\t':
        *out += "\\t";
        break;
      case '\b':
        *out += "\\b";
        break;
      case '\f':
        *out += "\\f";
        break;
      default:
        if (c < 0x20) {
          char buffer[8];
          std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
          *out += buffer;
        } else {
          *out += static_cast<char>(c);
        }
        break;
      }
    }
    *out += '"';
  }

  Type type_ = Type::Null;
  bool bool_ = false;
  double number_ = 0.0;
  std::string string_;
  std::vector<Json> items_;
  std::vector<std::pair<std::string, Json>> members_;
};

} // namespace lwbasic::dap
