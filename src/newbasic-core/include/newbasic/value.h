#pragma once

#include <cstdint>
#include <string>

namespace newbasic {

// GW-BASIC value. The five kinds mirror GW-BASIC storage types; all numeric
// values are stored as host double (plan: "math is double" - no emulation of
// GW-BASIC floating point), and the kind controls display precision.
class Value {
public:
  enum class Kind : std::uint8_t {
    Int,  // 32-bit integer, -2147483648..2147483647
    Long, // 32-bit integer (legacy & suffix; equivalent range to Int)
    Sgl,  // single precision (7 significant display digits)
    Dbl,  // double precision (16 significant display digits)
    Str,  // string (CP437 bytes)
  };

  Value() = default;

  static Value Int(std::int32_t v);
  static Value Long(std::int32_t v);
  static Value Sgl(double v);
  static Value Dbl(double v);
  static Value Str(std::string s);

  Kind kind() const { return kind_; }
  bool IsString() const { return kind_ == Kind::Str; }
  bool IsNumeric() const { return kind_ != Kind::Str; }
  bool IsIntegralKind() const { return kind_ == Kind::Int || kind_ == Kind::Long; }

  double AsNumber() const;             // valid for numeric kinds
  const std::string& AsString() const; // valid for Str

private:
  Value(Kind kind, double num, std::string str) : kind_(kind), num_(num), str_(std::move(str)) {}

  Kind kind_ = Kind::Sgl;
  double num_ = 0.0;
  std::string str_;
};

} // namespace newbasic
