// M8: PRINT USING. Expectations come from the reference's rules and worked
// examples (docs/gw-basic/22-reference-print-to-screen.md), not from whatever the
// implementation happens to print.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/using.h"
#include "test_helpers.h"

using newbasic::AppendUsing;
using newbasic::FormatUsingNumber;
using newbasic::FormatUsingString;
using newbasic::UsingStatus;
using newbasic::Value;

namespace {

std::string Number(const std::string& field, double value) {
  std::string out;
  const UsingStatus status = FormatUsingNumber(&out, field, value);
  REQUIRE(status == UsingStatus::Ok);
  return out;
}

std::string StringField(const std::string& field, const std::string& value) {
  std::string out;
  const UsingStatus status = FormatUsingString(&out, field, value);
  REQUIRE(status == UsingStatus::Ok);
  return out;
}

} // namespace

// Numeric fields: digit positions, decimal point, rounding. Values that fill the
// field print without a leading space (the reference's 987.65 example).
TEST_CASE("USING: numeric digit positions and rounding", "[using]") {
  // Reference: PRINT USING "###.##";987.654  ->  987.65
  CHECK(Number("###.##", 987.654) == "987.65");
  // Rounded as necessary, and the digit before the point always prints (as 0).
  CHECK(Number("##.##", 0.0) == " 0.00");
  CHECK(Number("##.##", 10.2) == "10.20");
  CHECK(Number("##.##", 66.789) == "66.79");
  CHECK(Number("##.##", 0.234) == " 0.23");
  // A format with no digit before the point prints just the fraction.
  CHECK(Number(".##", 0.5) == ".50");
  // Digit positions are always filled.
  CHECK(Number("#####", 42.0) == "   42");
  CHECK(Number("###", 0.0) == "  0");
  // A format that ends at the point still prints it.
  CHECK(Number("###.", 7.0) == "  7.");
}

// Signs: leading +, trailing -, and the default leading space/minus.
TEST_CASE("USING: numeric signs", "[using]") {
  // Reference: PRINT USING "+##.##";-68.95,2.4,55.6,-9
  //            -> -68.95 +2.40 +55.60 -0.90
  CHECK(Number("+##.##", -68.95) == "-68.95");
  CHECK(Number("+##.##", 2.4) == " +2.40");
  CHECK(Number("+##.##", 55.6) == "+55.60");
  // The reference's example output for -9 reads "-0.90", but its own rules
  // (digit before the point prints as 0, value rounded, not rescaled) give -9.00.
  // The OCR of that example is damaged; arithmetic wins here and the divergence is
  // noted rather than copied.
  CHECK(Number("+##.##", -9.0) == " -9.00");
  // Reference: PRINT USING "##.##-";-68.95,22.449,-7.01
  //            ->  68.95  22.45   7.01-
  CHECK(Number("##.##-", -68.95) == "68.95-");
  CHECK(Number("##.##-", 22.449) == "22.45 ");
  // Default: a leading space for positives where it fits, minus for negatives.
  CHECK(Number("###", 7.0) == "  7");
  CHECK(Number("###", -7.0) == " -7");
}

// Grouping commas to the left of the decimal point.
TEST_CASE("USING: digit grouping", "[using]") {
  CHECK(Number("#,###.##", 1234.5) == "1,234.50");
  CHECK(Number("#,###.##", 999.0) == "  999.00");
  // Overflow prints a percent sign and the digits without field grouping: the
  // value has already been marked as an error, and the reference does not say
  // whether grouping survives.
  CHECK(Number("#,###", 1234567.0) == "%1234567");
}

// String fields: ! (first character), \n spaces\ (2+n characters) and & (as-is).
TEST_CASE("USING: string fields", "[using]") {
  CHECK(StringField("!", "LOOK") == "L");
  // Width is 2 + the number of spaces between the backslashes.
  CHECK(StringField("\\ \\", "LOOK") == "LOO");   // two spaces' worth: 3
  CHECK(StringField("\\  \\", "LOOK") == "LOOK"); // 4
  // Shorter strings are left-justified and padded on the right.
  CHECK(StringField("\\  \\", "AB") == "AB  ");
  // Longer strings are truncated to the field.
  CHECK(StringField("!", "") == " ");
  CHECK(StringField("&", "LOOKOUT") == "LOOKOUT");
}

// The format string cycles: one field formats a whole list. Values that fill the
// field exactly run together, which is why the reference's own example puts
// literal spaces at the end of the format.
TEST_CASE("USING: the format cycles over a longer list", "[using]") {
  const std::string format = "##.##";
  std::size_t pos = 0;
  std::string out;
  for (const double value : {10.2, 5.3, 66.789, 0.234}) {
    REQUIRE(AppendUsing(&out, format, &pos, Value::Sgl(value)) == UsingStatus::Ok);
  }
  CHECK(out == "10.20 5.3066.79 0.23");
  // With a literal separator in the format, the same list reads cleanly. Each
  // value pads to its field (5) and then the three literal spaces follow.
  const std::string spaced = "##.##   ";
  pos = 0;
  out.clear();
  for (const double value : {10.2, 5.3, 66.789, 0.234}) {
    REQUIRE(AppendUsing(&out, spaced, &pos, Value::Sgl(value)) == UsingStatus::Ok);
  }
  CHECK(out == "10.20    5.30   66.79    0.23");
}

// Literals in the format are printed between items, and repeat when the format
// cycles.
TEST_CASE("USING: literal text in the format", "[using]") {
  std::size_t pos = 0;
  std::string out;
  REQUIRE(AppendUsing(&out, "###x", &pos, Value::Int(1)) == UsingStatus::Ok);
  REQUIRE(AppendUsing(&out, "###x", &pos, Value::Int(2)) == UsingStatus::Ok);
  // The literal after the last field is not printed by the last item: literals
  // belong between formatted fields.
  CHECK(out == "  1x  2");
  pos = 0;
  out.clear();
  REQUIRE(AppendUsing(&out, "###.", &pos, Value::Int(7)) == UsingStatus::Ok);
  CHECK(out == "  7.");
}

// The richer numeric fields: asterisk fill, dollar signs and exponential form.
// Each case records the interpretation where the reference's examples are damaged.
TEST_CASE("USING: asterisk fill, dollar signs and exponential fields", "[using]") {
  // `**` fills leading spaces with asterisks and adds two digit positions; the
  // reference example is "*12.4" for 12.39.
  CHECK(Number("**#.#", 12.39) == "*12.4");
  CHECK(Number("**#.#", 765.1) == "765.1"); // fills the field: no room for fill
  CHECK(Number("**#.#", -0.9) == "*-0.9");
  // `$$` puts the dollar sign immediately left of the digits, with the sign column
  // before it.
  CHECK(Number("$$###.##", 456.78) == " $456.78");
  // Interpretation: a negative with `$$` and no trailing sign puts the minus before
  // the dollar sign (the reference only says the minus must trail).
  CHECK(Number("$$###.##", -456.78) == "-$456.78");
  // `**$` combines both; the reference example is "***$2.34" for 2.34.
  CHECK(Number("**$##.##", 2.34) == "***$2.34");
  // `^^^^` is exponential: one sign column, one digit, then the fraction and E±dd.
  CHECK(Number("##.##^^^^", 234.56) == " 2.35E+02");
  CHECK(Number("##.##^^^^", 0.0) == " 0.00E+00");
  CHECK(Number("##.##^^^^", -1234.0) == "-1.23E+03");
  // No unsupported field types remain.
  std::string dummy;
  CHECK(FormatUsingNumber(&dummy, "**#", 5.0) == UsingStatus::Ok);
  CHECK(FormatUsingNumber(&dummy, "$$#", 5.0) == UsingStatus::Ok);
  CHECK(FormatUsingNumber(&dummy, "##^^^^", 5.0) == UsingStatus::Ok);
}

// End to end through the VM: PRINT USING statement.
TEST_CASE("USING: PRINT USING end to end", "[using]") {
  CHECK(lwtest::Row0("10 PRINT USING \"###.##\"; 987.654\n") == "987.65");
  CHECK(lwtest::Row0("10 PRINT USING \"+##.##\"; -68.95; 2.4\n") == "-68.95 +2.40");
  CHECK(lwtest::Row0("10 PRINT USING \"! !\"; \"LOOK\"; \"OUT\"\n") == "L O");
  // The format is a string expression, so a variable works.
  CHECK(lwtest::Row0("10 F$=\"###\":\n20 PRINT USING F$; 42\n") == " 42");
  // The richer fields work end to end too. With `**`, both the sign column and the
  // remaining padding are filled, so a one-digit value in "**#" prints "**5".
  CHECK(lwtest::Row0("10 PRINT USING \"**#\"; 5\n") == "**5");
  CHECK(lwtest::Row0("10 PRINT USING \"$$###.##\"; 12.5\n") == "  $12.50");
  // A type mismatch is the reference's Type mismatch (13), not a bare Illegal function call:
  // the latter sends the reader looking in the wrong place.
  CHECK(lwtest::RunErrorCode("10 PRINT USING \"###\"; \"x\"\n") == newbasic::kErrTypeMismatch);
  CHECK(lwtest::RunErrorCode("10 PRINT USING \"$$\"; \"x\"\n") == newbasic::kErrTypeMismatch);
}
