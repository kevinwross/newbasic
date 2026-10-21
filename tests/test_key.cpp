// M8: the KEY family. Semantics from the reference: key numbers 1-10 and 15-20,
// values truncated to 15 characters, a null value disabling the key, KEY ON
// showing the first six characters of each value on line 25, and KEY OFF making
// that line available to the program again.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "lwbasic/errors.h"
#include "test_helpers.h"

TEST_CASE("KEY: assignment and KEY LIST", "[key]") {
  const std::vector<std::string> rows =
      lwtest::RunRows("10 KEY 1,\"MENU\": KEY 2,\"RUN\"\n20 KEY LIST\n");
  CHECK(rows[0] == "F1 MENU");
  CHECK(rows[1] == "F2 RUN");
  CHECK(rows[2] == "F3"); // unassigned keys list empty (trailing space trimmed)
}

TEST_CASE("KEY: values are truncated to 15 characters", "[key]") {
  const std::vector<std::string> rows =
      lwtest::RunRows("10 KEY 1,\"ABCDEFGHIJKLMNOPQRSTUVWXYZ\"\n20 KEY LIST\n");
  CHECK(rows[0] == "F1 ABCDEFGHIJKLMNO"); // 15 characters
}

TEST_CASE("KEY ON shows the bar on the last line and reserves it", "[key]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 KEY 1,\"MENU\"\n"
                                                        "20 KEY ON\n"
                                                        "30 FOR I=1 TO 30\n"
                                                        "40 PRINT I\n"
                                                        "50 NEXT\n");
  // The bar keeps the key number and the value's first six characters, and program
  // output cannot overwrite it. All ten keys are laid out at width 80.
  CHECK(rows[24].rfind("1MENU", 0) == 0);
  CHECK(rows[24].find("10") != std::string::npos);
  CHECK(!rows[0].empty());
}

TEST_CASE("KEY ON lays out several keys eight columns apart", "[key]") {
  const std::vector<std::string> rows =
      lwtest::RunRows("10 KEY 1,\"MENU\": KEY 2,\"RUN\"\n20 KEY ON\n");
  CHECK(rows[24].rfind("1MENU   2RUN", 0) == 0);
}

TEST_CASE("KEY OFF clears the bar and gives the line back", "[key]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 KEY 1,\"MENU\"\n"
                                                        "20 KEY ON\n"
                                                        "30 KEY OFF\n"
                                                        "40 LOCATE 25,1: PRINT \"BOTTOM\"\n");
  // PRINT on the last line ends with a newline, which scrolls once, so the text
  // lands on the row above the bottom.
  CHECK(rows[23] == "BOTTOM");
}

TEST_CASE("KEY: a null value disables the key", "[key]") {
  const std::vector<std::string> rows =
      lwtest::RunRows("10 KEY 1,\"MENU\": KEY 1,\"\"\n20 KEY LIST\n");
  CHECK(rows[0] == "F1"); // disabled: nothing listed
}

TEST_CASE("KEY: out-of-range key numbers are Illegal function call", "[key]") {
  // 11-14 are outside the documented ranges.
  CHECK(lwtest::RunErrorCode("10 KEY 11,\"x\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 KEY 0,\"x\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 KEY 21,\"x\"\n") == lwbasic::kErrIllegalFunctionCall);
  // 15-20 are allowed.
  CHECK(lwtest::RunErrorCode("10 KEY 15,\"x\"\n") == 0);
  CHECK(lwtest::RunErrorCode("10 KEY 20,\"x\"\n") == 0);
}
