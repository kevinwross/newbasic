// M3: sequential file output (OPEN/CLOSE/PRINT#/WRITE#/EOF).
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

namespace {

// Unique per process: a leftover file cannot affect this run (plan.md 7b).
#define kFile lwtest::TempName("newbasic_tmp_io.txt")

std::string ReadAll(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string Path() {
  return kFile;
}

} // namespace

TEST_CASE("Sequential files: OPEN FOR OUTPUT with PRINT#", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, \"HELLO\"\n"
                  "30 PRINT #1, 42\n"
                  "40 CLOSE #1\n");
  // PRINT# writes what PRINT would display, with CRLF line ends.
  CHECK(ReadAll(kFile) == "HELLO\r\n 42 \r\n");
  std::remove(kFile);
}

TEST_CASE("Sequential files: WRITE# quotes strings and uses commas", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 WRITE #1, \"A\", 5\n"
                  "30 CLOSE #1\n");
  CHECK(ReadAll(kFile) == "\"A\",5\r\n");
  std::remove(kFile);
}

TEST_CASE("Sequential files: APPEND and the short OPEN form", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"O\", #1, \"" + Path() +
                  "\"\n"
                  "20 PRINT #1, \"ONE\"\n"
                  "30 CLOSE\n");
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR APPEND AS #1\n"
                  "20 PRINT #1, \"TWO\"\n"
                  "30 CLOSE #1\n");
  CHECK(ReadAll(kFile) == "ONE\r\nTWO\r\n");
  std::remove(kFile);
}

TEST_CASE("Sequential files: multiple items and zones", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, 1, 2\n"
                  "30 PRINT #1, \"A\";\"B\"\n"
                  "40 CLOSE #1\n");
  const std::string text = ReadAll(kFile);
  CHECK(text.find(" 1 ") == 0);
  CHECK(text.find("\r\nAB") != std::string::npos);
  std::remove(kFile);
}

TEST_CASE("Sequential files: EOF reports the end of an input file", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, \"DATA\"\n"
                  "30 CLOSE #1\n");
  CHECK(lwtest::Row0("10 OPEN \"" + Path() +
                     "\" FOR INPUT AS #1\n"
                     "20 PRINT EOF(1)\n"
                     "30 CLOSE #1\n") == " 0");
  std::remove(kFile);
}

TEST_CASE("Sequential files: errors", "[fileio]") {
  CHECK(lwtest::RunErrorCode("10 OPEN \"no_such_file_xyz.txt\" FOR INPUT AS #1\n") ==
        newbasic::kErrFileNotFound);
  CHECK(lwtest::RunErrorCode("10 PRINT #7, \"x\"\n") == newbasic::kErrBadFileNumber);
  CHECK(lwtest::RunErrorCode("10 CLOSE #7\n") == newbasic::kErrBadFileNumber);
  CHECK(lwtest::RunErrorCode("10 OPEN \"" + Path() + "\" FOR OUTPUT AS #0\n") ==
        newbasic::kErrBadFileNumber);
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() + "\" FOR OUTPUT AS #1\n20 CLOSE #1\n");
  CHECK(lwtest::RunErrorCode("10 OPEN \"" + Path() +
                             "\" FOR OUTPUT AS #1\n"
                             "20 OPEN \"" +
                             Path() + "\" FOR OUTPUT AS #1\n") == newbasic::kErrFileAlreadyOpen);
  std::remove(kFile);
}

TEST_CASE("Sequential files: INPUT # reads comma-separated fields", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, \"10,20\"\n"
                  "30 PRINT #1, \"30,40\"\n"
                  "40 CLOSE #1\n");
  CHECK(lwtest::Row0("10 OPEN \"" + Path() +
                     "\" FOR INPUT AS #1\n"
                     "20 INPUT #1, A, B\n"
                     "30 PRINT A;B\n"
                     "40 CLOSE #1\n") == " 10  20");
  std::remove(kFile);
}

TEST_CASE("Sequential files: LINE INPUT # keeps commas", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, \"a,b,c\"\n"
                  "30 CLOSE #1\n");
  CHECK(lwtest::Row0("10 OPEN \"" + Path() +
                     "\" FOR INPUT AS #1\n"
                     "20 LINE INPUT #1, L$\n"
                     "30 PRINT L$\n"
                     "40 CLOSE #1\n") == "a,b,c");
  std::remove(kFile);
}

TEST_CASE("Sequential files: reading past the end", "[fileio]") {
  std::remove(kFile);
  lwtest::RunRows("10 OPEN \"" + Path() +
                  "\" FOR OUTPUT AS #1\n"
                  "20 PRINT #1, \"1\"\n"
                  "30 CLOSE #1\n");
  CHECK(lwtest::RunErrorCode("10 OPEN \"" + Path() +
                             "\" FOR INPUT AS #1\n"
                             "20 INPUT #1, A\n"
                             "30 INPUT #1, B\n") == newbasic::kErrInputPastEnd);
  CHECK(lwtest::RunErrorCode("10 INPUT #3, A\n") == newbasic::kErrBadFileNumber);
  std::remove(kFile);
}
