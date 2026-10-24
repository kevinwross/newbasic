#include "newbasic/value.h"

namespace newbasic {

Value Value::Int(std::int32_t v) {
  return Value(Kind::Int, v, {});
}
Value Value::Long(std::int32_t v) {
  return Value(Kind::Long, v, {});
}
Value Value::Sgl(double v) {
  return Value(Kind::Sgl, v, {});
}
Value Value::Dbl(double v) {
  return Value(Kind::Dbl, v, {});
}
Value Value::Str(std::string s) {
  return Value(Kind::Str, 0.0, std::move(s));
}

double Value::AsNumber() const {
  return kind_ == Kind::Str ? 0.0 : num_;
}

const std::string& Value::AsString() const {
  return str_;
}

} // namespace newbasic
