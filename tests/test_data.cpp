// M2: DATA / READ / RESTORE.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "test_helpers.h"

using lwtest::Row0;
using lwtest::RunErrorCode;

TEST_CASE("DATA/READ: numbers and strings", "[data]") {
  CHECK(Row0("10 DATA 1,2,3\n20 READ A,B,C\n30 PRINT A;B;C\n") == " 1  2  3");
  CHECK(Row0("10 DATA hello,world\n20 READ A$,B$\n30 PRINT A$;B$\n") == "helloworld");
  CHECK(Row0("10 DATA \"a,b\",c\n20 READ A$,B$\n30 PRINT A$;B$\n") == "a,bc");
  CHECK(Row0("10 DATA 1.5\n20 READ A\n30 PRINT A\n") == " 1.5");
  // A numeric item converted into an integer variable rounds.
  CHECK(Row0("10 DATA 2.6\n20 READ A%\n30 PRINT A%\n") == " 3");
  // Numbers read into a string variable keep their text.
  CHECK(Row0("10 DATA 5\n20 READ A$\n30 PRINT A$\n") == "5");
  // DATA anywhere in the program feeds READ (it is collected up front).
  CHECK(Row0("10 READ X\n20 PRINT X\n30 DATA 42\n") == " 42");
  // Several READ statements consume the pool in order.
  CHECK(Row0("10 DATA 1,2,3,4\n20 READ A,B\n30 READ C\n40 PRINT A+B+C\n") == " 6");
  // DATA lines are inert.
  CHECK(Row0("10 DATA 1\n20 PRINT \"OK\"\n") == "OK");
  // An omitted item reads as zero.
  CHECK(Row0("10 DATA 1,,3\n20 READ A,B,C\n30 PRINT A;B;C\n") == " 1  0  3");
}

TEST_CASE("DATA/READ: RESTORE", "[data]") {
  CHECK(Row0("10 DATA 1,2\n20 READ A,B\n30 RESTORE\n40 READ C\n50 PRINT C\n") == " 1");
  CHECK(Row0("10 DATA 1\n20 DATA 2\n30 READ A\n40 RESTORE 20\n50 READ B\n60 PRINT B\n") == " 2");
  CHECK(Row0("10 DATA 1\n20 DATA 2\n30 READ A: RESTORE 10: READ B\n40 PRINT A;B\n") == " 1  1");
  CHECK(RunErrorCode("10 DATA 1\n20 READ A,B\n") == newbasic::kErrOutOfData);
  CHECK(RunErrorCode("10 DATA x\n20 READ A\n") == newbasic::kErrSyntax);
  CHECK(RunErrorCode("10 DATA 1\n20 RESTORE 99\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(lwtest::CompileCode("10 DATA 1,,2\n") == 0);
}

TEST_CASE("DATA/READ: RESTORE to a line, and back to the start", "[data]") {
  CHECK(Row0("10 DATA 1,2\n20 DATA 3,4\n30 RESTORE 20\n40 READ A,B\n"
             "50 IF A = 3 AND B = 4 THEN PRINT \"OK\";\n") == "OK");
  CHECK(Row0("10 DATA 5\n20 READ A\n30 RESTORE\n40 READ B\n"
             "50 IF A = 5 AND B = 5 THEN PRINT \"OK\";\n") == "OK");
  // After a RESTORE the walk continues from the restored point, not the original one.
  CHECK(Row0("10 DATA 1,2,3\n20 READ A\n30 RESTORE 10\n40 READ B,C\n"
             "50 IF A = 1 AND B = 1 AND C = 2 THEN PRINT \"OK\";\n") == "OK");
}

TEST_CASE("DATA/READ: item kinds and conversion", "[data]") {
  // Negatives and exponential notation are numeric items.
  CHECK(Row0("10 DATA -1, 2.5E3\n20 READ A,B\n"
             "30 IF A = -1 AND B = 2500 THEN PRINT \"OK\";\n") == "OK");
  // A numeric item read into a string variable keeps its text.
  CHECK(Row0("10 DATA 42\n20 READ S$\n30 IF S$ = \"42\" THEN PRINT \"OK\";\n") == "OK");
  // Text in a numeric slot is a syntax error, as in the reference.
  CHECK(RunErrorCode("10 DATA \"abc\"\n20 READ A\n") == newbasic::kErrSyntax);
}
