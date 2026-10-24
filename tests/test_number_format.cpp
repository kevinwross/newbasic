// GW-BASIC-style PRINT number formatting (plan risk #2: isolated module
// with its own test suite).
//
// Expected values are GW-BASIC display rules: a leading space (or '-') for
// the sign, 7 significant digits for Single, 16 for Double, no leading zero
// before '.', and E+/-xx outside the fixed-point budget.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/number_format.h"
#include "newbasic/value.h"

using newbasic::Value;

TEST_CASE("FormatNumber: integers", "[format]") {
  CHECK(newbasic::FormatNumber(Value::Int(5)) == " 5");
  CHECK(newbasic::FormatNumber(Value::Int(-5)) == "-5");
  CHECK(newbasic::FormatNumber(Value::Int(0)) == " 0");
  CHECK(newbasic::FormatNumber(Value::Int(32767)) == " 32767");
  CHECK(newbasic::FormatNumber(Value::Int(-32768)) == "-32768");
  CHECK(newbasic::FormatNumber(Value::Int(2147483647)) == " 2147483647");
  CHECK(newbasic::FormatNumber(Value::Int(-2147483647 - 1)) == "-2147483648");
  CHECK(newbasic::FormatNumber(Value::Long(123456)) == " 123456");
  CHECK(newbasic::FormatNumber(Value::Long(-999999)) == "-999999");
  CHECK(newbasic::FormatNumber(Value::Long(0)) == " 0");
}

TEST_CASE("FormatNumber: single precision (7 significant digits)", "[format]") {
  // Fixed point
  CHECK(newbasic::FormatNumber(Value::Sgl(3.14159)) == " 3.14159");
  CHECK(newbasic::FormatNumber(Value::Sgl(-3.14159)) == "-3.14159");
  CHECK(newbasic::FormatNumber(Value::Sgl(0.5)) == " .5");
  CHECK(newbasic::FormatNumber(Value::Sgl(1.0 / 3.0)) == " .3333333");
  CHECK(newbasic::FormatNumber(Value::Sgl(0.1 + 0.2)) == " .3");
  CHECK(newbasic::FormatNumber(Value::Sgl(10.0)) == " 10");
  CHECK(newbasic::FormatNumber(Value::Sgl(0.000001)) == " .000001");
  CHECK(newbasic::FormatNumber(Value::Sgl(-0.00001)) == "-.00001");
  CHECK(newbasic::FormatNumber(Value::Sgl(250000.0)) == " 250000");
  CHECK(newbasic::FormatNumber(Value::Sgl(9999999.0)) == " 9999999");
  // Rounding (ties-to-even at the 7th significant digit)
  CHECK(newbasic::FormatNumber(Value::Sgl(1000000.5)) == " 1000000");
  CHECK(newbasic::FormatNumber(Value::Sgl(9.9999999)) == " 10");
  // Exponential form (outside the 7-digit fixed budget)
  CHECK(newbasic::FormatNumber(Value::Sgl(12345678.0)) == " 1.234568E+07");
  CHECK(newbasic::FormatNumber(Value::Sgl(0.0000005)) == " 5E-07");
  CHECK(newbasic::FormatNumber(Value::Sgl(1.0e10)) == " 1E+10");
  CHECK(newbasic::FormatNumber(Value::Sgl(-1.0e10)) == "-1E+10");
  // Zero
  CHECK(newbasic::FormatNumber(Value::Sgl(0.0)) == " 0");
  CHECK(newbasic::FormatNumber(Value::Sgl(-0.0)) == " 0");
}

TEST_CASE("FormatNumber: double precision (16 significant digits)", "[format]") {
  CHECK(newbasic::FormatNumber(Value::Dbl(22.5)) == " 22.5");
  CHECK(newbasic::FormatNumber(Value::Dbl(1.0 / 3.0)) == " .3333333333333333");
  CHECK(newbasic::FormatNumber(Value::Dbl(12345678901234.5)) == " 12345678901234.5");
  CHECK(newbasic::FormatNumber(Value::Dbl(1.5e-10)) == " .00000000015");
  CHECK(newbasic::FormatNumber(Value::Dbl(1.0e20)) == " 1E+20");
  CHECK(newbasic::FormatNumber(Value::Dbl(-0.0)) == " 0");
}

TEST_CASE("FormatForPrint: strings", "[format]") {
  CHECK(newbasic::FormatForPrint(Value::Str("HI")) == "HI");
  CHECK(newbasic::FormatForPrint(Value::Str("")) == "");
  CHECK(newbasic::FormatForPrint(Value::Int(7)) == " 7");
}
