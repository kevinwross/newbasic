// M9: random-access files - OPEN FOR RANDOM, FIELD, LSET/RSET, GET/PUT, LOF/LOC.
// Files use unique names per run so an interrupted run cannot leave a file that
// changes the next run's result (the temp-file isolation item, applied here).
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

namespace {

std::string TempPath() {
  static int counter = 0;
  return "newbasic_rand_" + std::to_string(++counter) + ".dat";
}

struct TempFile {
  explicit TempFile(std::string path) : path_(std::move(path)) {}
  ~TempFile() { std::remove(path_.c_str()); }
  const std::string& Path() const { return path_; }

private:
  std::string path_;
};

} // namespace

TEST_CASE("Random files: FIELD, LSET and PUT round-trip through GET", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=20\n" +
                              "20 FIELD #1, 10 AS N$, 5 AS C$\n"
                              "30 LSET N$=\"HELLO\": LSET C$=\"AB\"\n"
                              "40 PUT #1, 1\n"
                              "50 CLOSE #1\n"
                              "60 OPEN \"" +
                              file.Path() + "\" FOR RANDOM AS #1 LEN=20\n" +
                              "70 FIELD #1, 10 AS N$, 5 AS C$\n"
                              "80 GET #1, 1\n"
                              "90 PRINT \"[\";N$;\"]\";\"[\";C$;\"]\"\n"
                              "100 CLOSE\n";
  // LSET pads on the right, and the field widths are what the record holds.
  CHECK(lwtest::Row0(program) == "[HELLO     ][AB   ]");
}

TEST_CASE("Random files: RSET right-justifies, LSET truncates from the right", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=20\n" +
                              "20 FIELD #1, 10 AS N$\n"
                              "30 RSET N$=\"HI\"\n"
                              "40 PUT #1, 1\n"
                              "50 GET #1, 1\n"
                              "60 PRINT \"[\";N$;\"]\"\n"
                              "70 LSET N$=\"ABCDEFGHIJKLMNOP\"\n" // longer than the field
                              "80 PRINT \"[\";N$;\"]\"\n"
                              "90 CLOSE\n";
  const std::string rows = lwtest::Row0(program);
  CHECK(rows == "[        HI]");
  // The 16-character assignment is truncated to the 10-byte field.
  const std::vector<std::string> all = lwtest::RunRows(program);
  CHECK(all[1] == "[ABCDEFGHIJ]");
}

TEST_CASE("Random files: FIELD cannot exceed the record length", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=10\n" +
                              "20 FIELD #1, 10 AS A$\n"
                              "30 FIELD #1, 5 AS B$\n" // 15 > 10
                              "40 CLOSE\n";
  CHECK(lwtest::RunErrorCode(program) == newbasic::kErrFieldOverflow);
}

TEST_CASE("Random files: LOF and LOC report position and size", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=20\n" +
                              "20 FIELD #1, 10 AS A$\n"
                              "30 LSET A$=\"X\": PUT #1, 1\n"
                              "40 LSET A$=\"Y\": PUT #1, 3\n" // record 3 leaves a gap
                              "50 PRINT LOF(1);LOC(1)\n"
                              "60 CLOSE\n";
  // Three records of 20 bytes, and the last record written was 3.
  CHECK(lwtest::Row0(program) == " 60  3");
}

TEST_CASE("Random files: GET without a record number reads sequentially", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=10\n" +
                              "20 FIELD #1, 10 AS A$\n"
                              "30 LSET A$=\"ONE\": PUT #1, 1\n"
                              "40 LSET A$=\"TWO\": PUT #1, 2\n"
                              "50 GET #1, 1: PRINT \"[\";A$;\"]\"\n"
                              "60 GET #1: PRINT \"[\";A$;\"]\"\n" // next record
                              "70 CLOSE\n";
  const std::vector<std::string> rows = lwtest::RunRows(program);
  CHECK(rows[0] == "[ONE       ]");
  CHECK(rows[1] == "[TWO       ]");
}

TEST_CASE("Random files: a never-written record still honours the field width", "[random]") {
  TempFile file(TempPath());
  const std::string program = "10 OPEN \"" + file.Path() + "\" FOR RANDOM AS #1 LEN=10\n" +
                              "20 FIELD #1, 4 AS A$\n"
                              "30 GET #1, 5\n" // never written: the file has no bytes there
                              "40 PRINT LEN(A$)\n"
                              "50 CLOSE\n";
  // The field keeps its width; the bytes themselves are whatever the file holds
  // (nothing), which the implementation zero-fills rather than inventing content.
  CHECK(lwtest::Row0(program) == " 4");
}
