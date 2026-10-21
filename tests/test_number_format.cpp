// GW-BASIC-style PRINT number formatting (plan risk #2: isolated module
// with its own test suite).
//
// Expected values are GW-BASIC display rules: a leading space (or '-') for
// the sign, 7 significant digits for Single, 16 for Double, no leading zero
// before '.', and E+/-xx outside the fixed-point budget.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "lwbasic/number_format.h"
#include "lwbasic/value.h"

using lwbasic::Value;

TEST_CASE("FormatNumber: integers", "[format]") {
  CHECK(lwbasic::FormatNumber(Value::Int(5)) == " 5");
  CHECK(lwbasic::FormatNumber(Value::Int(-5)) == "-5");
  CHECK(lwbasic::FormatNumber(Value::Int(0)) == " 0");
  CHECK(lwbasic::FormatNumber(Value::Int(32767)) == " 32767");
  CHECK(lwbasic::FormatNumber(Value::Int(-32768)) == "-32768");
  CHECK(lwbasic::FormatNumber(Value::Long(123456)) == " 123456");
  CHECK(lwbasic::FormatNumber(Value::Long(-999999)) == "-999999");
  CHECK(lwbasic::FormatNumber(Value::Long(0)) == " 0");
}

TEST_CASE("FormatNumber: single precision (7 significant digits)", "[format]") {
  // Fixed point
  CHECK(lwbasic::FormatNumber(Value::Sgl(3.14159)) == " 3.14159");
  CHECK(lwbasic::FormatNumber(Value::Sgl(-3.14159)) == "-3.14159");
  CHECK(lwbasic::FormatNumber(Value::Sgl(0.5)) == " .5");
  CHECK(lwbasic::FormatNumber(Value::Sgl(1.0 / 3.0)) == " .3333333");
  CHECK(lwbasic::FormatNumber(Value::Sgl(0.1 + 0.2)) == " .3");
  CHECK(lwbasic::FormatNumber(Value::Sgl(10.0)) == " 10");
  CHECK(lwbasic::FormatNumber(Value::Sgl(0.000001)) == " .000001");
  CHECK(lwbasic::FormatNumber(Value::Sgl(-0.00001)) == "-.00001");
  CHECK(lwbasic::FormatNumber(Value::Sgl(250000.0)) == " 250000");
  CHECK(lwbasic::FormatNumber(Value::Sgl(9999999.0)) == " 9999999");
  // Rounding (ties-to-even at the 7th significant digit)
  CHECK(lwbasic::FormatNumber(Value::Sgl(1000000.5)) == " 1000000");
  CHECK(lwbasic::FormatNumber(Value::Sgl(9.9999999)) == " 10");
  // Exponential form (outside the 7-digit fixed budget)
  CHECK(lwbasic::FormatNumber(Value::Sgl(12345678.0)) == " 1.234568E+07");
  CHECK(lwbasic::FormatNumber(Value::Sgl(0.0000005)) == " 5E-07");
  CHECK(lwbasic::FormatNumber(Value::Sgl(1.0e10)) == " 1E+10");
  CHECK(lwbasic::FormatNumber(Value::Sgl(-1.0e10)) == "-1E+10");
  // Zero
  CHECK(lwbasic::FormatNumber(Value::Sgl(0.0)) == " 0");
  CHECK(lwbasic::FormatNumber(Value::Sgl(-0.0)) == " 0");
}

TEST_CASE("FormatNumber: double precision (16 significant digits)", "[format]") {
  CHECK(lwbasic::FormatNumber(Value::Dbl(22.5)) == " 22.5");
  CHECK(lwbasic::FormatNumber(Value::Dbl(1.0 / 3.0)) == " .3333333333333333");
  CHECK(lwbasic::FormatNumber(Value::Dbl(12345678901234.5)) == " 12345678901234.5");
  CHECK(lwbasic::FormatNumber(Value::Dbl(1.5e-10)) == " .00000000015");
  CHECK(lwbasic::FormatNumber(Value::Dbl(1.0e20)) == " 1E+20");
  CHECK(lwbasic::FormatNumber(Value::Dbl(-0.0)) == " 0");
}

TEST_CASE("FormatForPrint: strings", "[format]") {
  CHECK(lwbasic::FormatForPrint(Value::Str("HI")) == "HI");
  CHECK(lwbasic::FormatForPrint(Value::Str("")) == "");
  CHECK(lwbasic::FormatForPrint(Value::Int(7)) == " 7");
}
