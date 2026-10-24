// M9: the binary conversions MKI$/CVI, MKS$/CVS and MKD$/CVD. These are the
// primitives random-access records are built from, so they are tested directly
// and through the VM.
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include "newbasic/errors.h"
#include "newbasic/field.h"
#include "test_helpers.h"

using newbasic::CvdFromString;
using newbasic::CviFromString;
using newbasic::CvsFromString;
using newbasic::MkdFromDouble;
using newbasic::MkiFromInt;
using newbasic::MksFromSingle;

TEST_CASE("Field: MKI$ is two bytes, low byte first", "[field]") {
  const std::string one = MkiFromInt(1);
  REQUIRE(one.size() == 2);
  CHECK(static_cast<unsigned char>(one[0]) == 1);
  CHECK(static_cast<unsigned char>(one[1]) == 0);
  const std::string big = MkiFromInt(0x1234);
  CHECK(static_cast<unsigned char>(big[0]) == 0x34);
  CHECK(static_cast<unsigned char>(big[1]) == 0x12);
  // Negative values are two's complement.
  const std::string minus_one = MkiFromInt(-1);
  CHECK(static_cast<unsigned char>(minus_one[0]) == 0xFF);
  CHECK(static_cast<unsigned char>(minus_one[1]) == 0xFF);
}

TEST_CASE("Field: CVI inverts MKI$", "[field]") {
  for (const std::int16_t value :
       {static_cast<std::int16_t>(0), static_cast<std::int16_t>(1), static_cast<std::int16_t>(-1),
        static_cast<std::int16_t>(32767), static_cast<std::int16_t>(-32768),
        static_cast<std::int16_t>(1234)}) {
    std::int16_t back = 0;
    REQUIRE(CviFromString(MkiFromInt(value), &back));
    CHECK(back == value);
  }
  // A string of the wrong length cannot be converted.
  std::int16_t ignored = 0;
  CHECK_FALSE(CviFromString("x", &ignored));
  CHECK_FALSE(CviFromString("xyz", &ignored));
}

TEST_CASE("Field: MKS$/CVS and MKD$/CVD round-trip", "[field]") {
  // Exactly representable in MBF single (24-bit significand).
  for (const double value : {0.0, 1.0, -1.0, 1.5, 1234.5, -0.25}) {
    double back = 0.0;
    REQUIRE(CvsFromString(MksFromSingle(value), &back));
    CHECK(back == value);
    REQUIRE(CvdFromString(MkdFromDouble(value), &back));
    CHECK(back == value);
  }
  // Larger magnitudes round-trip through double, whose significand has 55 bits.
  for (const double value : {1.0e20, -3.5e30}) {
    double back = 0.0;
    REQUIRE(CvdFromString(MkdFromDouble(value), &back));
    CHECK(back == value);
  }
  CHECK(MksFromSingle(1.0).size() == 4);
  CHECK(MkdFromDouble(1.0).size() == 8);
  // Double holds more precision than single: a value that single rounds still
  // round-trips exactly through double.
  const double precise = 1.2345678901234;
  double back = 0.0;
  REQUIRE(CvdFromString(MkdFromDouble(precise), &back));
  CHECK(back == precise);
  // Wrong lengths are rejected.
  double ignored = 0.0;
  CHECK_FALSE(CvsFromString("abc", &ignored));
  CHECK_FALSE(CvdFromString("abcd", &ignored));
}

TEST_CASE("Field: the conversions are reachable from BASIC", "[field]") {
  CHECK(lwtest::Row0("10 PRINT LEN(MKI$(1));LEN(MKS$(1));LEN(MKD$(1))\n") == " 2  4  8");
  CHECK(lwtest::Row0("10 PRINT CVI(MKI$(1234))\n") == " 1234");
  // MKI$ is a two-byte field conversion even though % is 32-bit now.
  CHECK(lwtest::RunErrorCode("10 PRINT MKI$(40000%)\n") == newbasic::kErrOverflow);
  CHECK(lwtest::Row0("10 PRINT CVI(MKI$(-1))\n") == "-1");
  CHECK(lwtest::Row0("10 PRINT CVS(MKS$(1.5))\n") == " 1.5");
  CHECK(lwtest::Row0("10 PRINT CVD(MKD$(1.5))\n") == " 1.5");
  // Low byte first, visible from BASIC.
  CHECK(lwtest::Row0("10 S$=MKI$(1)\n20 PRINT ASC(S$);ASC(MID$(S$,2,1))\n") == " 1  0");
  // Wrong length is an Illegal function call.
  CHECK(lwtest::RunErrorCode("10 PRINT CVI(\"x\")\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT CVS(\"ab\")\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PRINT CVD(\"abcd\")\n") == newbasic::kErrIllegalFunctionCall);
}
