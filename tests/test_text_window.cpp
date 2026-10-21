// M8: VIEW PRINT (the text window), CLS variants and WIDTH.
// Expectations come from the reference: VIEW PRINT confines CLS, LOCATE, print and
// scrolling to the window; CLS 0 clears everything, CLS 2 only the text window;
// WIDTH sets the column count.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "test_helpers.h"

TEST_CASE("Window: VIEW PRINT confines scrolling", "[window]") {
  // A four-line window at rows 5..8 (1-based). Ten printed lines scroll six times
  // (each PRINT ends with a newline), so the survivors are 8, 9, 10 with the
  // window's last row left blank by the final newline.
  const std::vector<std::string> rows = lwtest::RunRows("10 VIEW PRINT 5 TO 8\n"
                                                        "20 FOR I=1 TO 10\n"
                                                        "30 PRINT I\n"
                                                        "40 NEXT\n");
  REQUIRE(rows.size() == 25);
  for (int row = 0; row < 4; ++row)
    CHECK(rows[static_cast<std::size_t>(row)].empty());
  CHECK(rows[4] == " 8");
  CHECK(rows[5] == " 9");
  CHECK(rows[6] == " 10");
  CHECK(rows[7].empty());
  for (int row = 8; row < 25; ++row)
    CHECK(rows[static_cast<std::size_t>(row)].empty());
}

TEST_CASE("Window: VIEW PRINT confines LOCATE", "[window]") {
  // The cursor cannot leave the window, so LOCATE 1,1 lands on the window's top
  // row instead of the screen's.
  const std::vector<std::string> rows =
      lwtest::RunRows("10 VIEW PRINT 5 TO 8\n20 LOCATE 1,1\n30 PRINT \"A\"\n");
  CHECK(rows[0].empty());
  CHECK(rows[4] == "A");
}

TEST_CASE("Window: CLS 2 clears only the window, CLS 0 clears everything", "[window]") {
  // The text outside the window is written *before* VIEW PRINT, because once a
  // window is set the cursor cannot leave it.
  const std::vector<std::string> window_only = lwtest::RunRows("10 PRINT \"OUTSIDE\"\n"
                                                               "20 VIEW PRINT 5 TO 8\n"
                                                               "30 LOCATE 5,1: PRINT \"INSIDE\"\n"
                                                               "40 CLS 2\n");
  CHECK(window_only[0] == "OUTSIDE"); // outside the window: untouched
  for (int row = 4; row < 8; ++row)
    CHECK(window_only[static_cast<std::size_t>(row)].empty());

  const std::vector<std::string> everything = lwtest::RunRows("10 PRINT \"X\"\n20 CLS 0\n");
  for (const std::string& row : everything)
    CHECK(row.empty());
}

TEST_CASE("Window: VIEW PRINT without arguments restores the whole screen", "[window]") {
  const std::vector<std::string> rows = lwtest::RunRows("10 VIEW PRINT 5 TO 8\n"
                                                        "20 VIEW PRINT\n"
                                                        "30 FOR I=1 TO 30\n"
                                                        "40 PRINT I\n"
                                                        "50 NEXT\n");
  CHECK(rows[0] == " 7"); // 30 newlines from the top: survivors are 7..30
  CHECK(rows[23] == " 30");
  CHECK(rows[24].empty());
}

TEST_CASE("Window: WIDTH narrows the grid and printing wraps there", "[window]") {
  const std::vector<std::string> rows =
      lwtest::RunRows("10 WIDTH 40\n20 PRINT STRING$(45,\"x\")\n");
  REQUIRE(rows.size() == 25);
  CHECK(rows[0].size() == 40); // wrapped at the new width
  CHECK(rows[0] == std::string(40, 'x'));
  CHECK(rows[1] == std::string(5, 'x'));
}

TEST_CASE("Window: invalid ranges and widths are Illegal function call", "[window]") {
  CHECK(lwtest::RunErrorCode("10 VIEW PRINT 8 TO 5\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 VIEW PRINT 1 TO 30\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 VIEW PRINT 0 TO 4\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 WIDTH 200\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 WIDTH 0\n") == newbasic::kErrIllegalFunctionCall);
}
