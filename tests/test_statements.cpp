// M1 statements: PRINT, control flow, screen operations and error paths.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "test_helpers.h"

using lwtest::CompileCode;
using lwtest::Row0;
using lwtest::RunErrorCode;
using lwtest::RunRows;

TEST_CASE("Statements: PRINT lists and separators", "[stmt][print]") {
  CHECK(Row0("10 PRINT \"HI\"\n") == "HI");
  CHECK(Row0("10 PRINT 1;2\n") == " 1  2");
  CHECK(Row0("10 PRINT -5\n") == "-5");
  CHECK(Row0("10 PRINT 1 2\n") == " 1  2"); // a space acts as a semicolon
  CHECK(Row0("10 PRINT \"A\";\"B\"\n") == "AB");
  CHECK(Row0("10 ? 42\n") == " 42");
  const std::vector<std::string> rows = RunRows("10 PRINT \"A\",\"B\"\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "A             B"); // second item at print zone 14
}

TEST_CASE("Statements: FOR/NEXT", "[stmt][for]") {
  std::vector<std::string> rows = RunRows("10 FOR I=1 TO 3\n20 PRINT I\n30 NEXT I\n");
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == " 1");
  CHECK(rows[1] == " 2");
  CHECK(rows[2] == " 3");

  rows = RunRows("10 FOR I=1 TO 3: PRINT I: NEXT I\n");
  REQUIRE(rows.size() >= 3);
  CHECK(rows[2] == " 3");

  rows = RunRows("10 FOR I=5 TO 1 STEP -2: PRINT I: NEXT I\n");
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == " 5");
  CHECK(rows[1] == " 3");
  CHECK(rows[2] == " 1");

  // A loop whose start is already past the end never runs.
  rows = RunRows("10 FOR I=5 TO 1: PRINT \"NO\": NEXT I: PRINT \"DONE\"\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "DONE");

  // Nested loops and NEXT with explicit variables.
  rows = RunRows("10 FOR I=1 TO 2: FOR J=1 TO 2: PRINT I;J: NEXT J: NEXT I\n");
  REQUIRE(rows.size() >= 4);
  CHECK(rows[0] == " 1  1");
  CHECK(rows[1] == " 1  2");
  CHECK(rows[2] == " 2  1");
  CHECK(rows[3] == " 2  2");

  // The loop variable keeps its declared type.
  rows = RunRows("10 FOR I%=1 TO 2: PRINT I%: NEXT I%\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == " 1");
  CHECK(rows[1] == " 2");

  // The variable holds the first out-of-range value after the loop.
  CHECK(Row0("10 FOR I=1 TO 3: NEXT I: PRINT I\n") == " 4");
}

TEST_CASE("Statements: WHILE/WEND", "[stmt][while]") {
  const std::vector<std::string> rows =
      RunRows("10 A=0\n20 WHILE A<3\n30 A=A+1\n40 PRINT A\n50 WEND\n");
  REQUIRE(rows.size() >= 3);
  CHECK(rows[0] == " 1");
  CHECK(rows[1] == " 2");
  CHECK(rows[2] == " 3");
  CHECK(Row0("10 WHILE 0: PRINT \"NO\": WEND: PRINT \"DONE\"\n") == "DONE");
}

TEST_CASE("Statements: IF/THEN/ELSE", "[stmt][if]") {
  CHECK(Row0("10 IF 1 THEN PRINT \"Y\" ELSE PRINT \"N\"\n") == "Y");
  CHECK(Row0("10 IF 0 THEN PRINT \"Y\" ELSE PRINT \"N\"\n") == "N");
  CHECK(Row0("10 IF 1 THEN PRINT \"Y\"\n") == "Y");
  CHECK(Row0("10 IF 0 THEN PRINT \"Y\"\n") == "");
  CHECK(Row0("10 IF 1 THEN PRINT \"A\": PRINT \"B\"\n20 PRINT \"C\"\n") == "A");
  const std::vector<std::string> rows =
      RunRows("10 IF 1 THEN 30\n20 PRINT \"NO\": END\n30 PRINT \"YES\"\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "YES");
  CHECK(Row0("10 IF 2>1 GOTO 100\n20 PRINT \"NO\": END\n100 PRINT \"YES\"\n") == "YES");
}

TEST_CASE("Statements: GOTO, GOSUB and ON ... GOTO/GOSUB", "[stmt][jump]") {
  std::vector<std::string> rows = RunRows("10 GOTO 30\n20 PRINT \"NO\"\n30 PRINT \"YES\"\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "YES");

  rows = RunRows("10 GOSUB 100: PRINT \"BACK\": END\n100 PRINT \"SUB\": RETURN\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "SUB");
  CHECK(rows[1] == "BACK");

  rows = RunRows("10 ON 2 GOTO 100,200\n20 END\n100 PRINT \"ONE\": END\n200 PRINT \"TWO\": END\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "TWO");

  // Out of range (but <= 255) continues with the next statement.
  rows = RunRows("10 ON 5 GOTO 100,200: PRINT \"NEXT\": END\n100 PRINT \"NO\": END\n"
                 "200 PRINT \"NO\": END\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "NEXT");
  CHECK(RunErrorCode("10 ON -1 GOTO 100,200\n100 END\n") == newbasic::kErrIllegalFunctionCall);
  CHECK(RunErrorCode("10 ON 2 GOSUB 100,200: END\n100 RETURN\n200 RETURN\n") == 0);
}

TEST_CASE("Statements: comments, END and STOP", "[stmt]") {
  CHECK(Row0("10 REM nothing here\n20 PRINT \"OK\"\n") == "OK");
  CHECK(Row0("10 'note\n20 PRINT \"OK\"\n") == "OK");
  CHECK(Row0("10 PRINT \"A\": REM x\n") == "A");
  CHECK(RunRows("10 PRINT \"A\"\n20 END\n30 PRINT \"B\"\n")[0] == "A");
  const std::vector<std::string> rows = RunRows("10 PRINT \"A\"\n20 STOP\n30 PRINT \"B\"\n");
  REQUIRE(!rows.empty());
  CHECK(rows[0] == "A");
}

TEST_CASE("Statements: CLS, LOCATE and COLOR", "[stmt][screen]") {
  std::vector<std::string> rows =
      RunRows("10 PRINT \"AB\"\n20 CLS\n30 LOCATE 2,10\n40 PRINT \"X\"\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0].empty());
  CHECK(rows[1] == "         X");

  // COLOR sets the current attribute; clear screens use it too.
  newbasic::Program program;
  newbasic::CompileError error;
  newbasic::LineStore store;
  REQUIRE(newbasic::LineStore::ParseTextBas("10 COLOR 14,1\n20 PRINT \"X\"\n", &store, nullptr));
  REQUIRE(newbasic::Compile(store, &program, &error));
  newbasic::screen::HeadlessScreen screen;
  newbasic::Vm vm(screen);
  vm.Load(std::move(program));
  CHECK(vm.Run().status == newbasic::RunResult::Status::Done);
  CHECK(screen.ColorFg() == 14);
  CHECK(screen.ColorBg() == 1);
  CHECK(screen.AttrFg(screen.CellAttrAt(0, 0)) == 14);
  CHECK(screen.AttrBg(screen.CellAttrAt(0, 0)) == 1);
}

TEST_CASE("Statements: compile-time errors", "[stmt][errors]") {
  CHECK(CompileCode("10 NEXT I\n") == newbasic::kErrNextWithoutFor);
  CHECK(CompileCode("10 FOR I=1 TO 3\n") == newbasic::kErrForWithoutNext);
  CHECK(CompileCode("10 WEND\n") == newbasic::kErrWendWithoutWhile);
  CHECK(CompileCode("10 WHILE 1\n20 PRINT 1\n") == newbasic::kErrWhileWithoutWend);
  CHECK(CompileCode("10 PRINT\n") == 0);
  CHECK(CompileCode("10 FOO 1\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 PRINT 1+\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 PRINT (1\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 FOR I=1 TO\n") == newbasic::kErrSyntax);
  CHECK(CompileCode("10 PRINT ABS(1,2)\n") == newbasic::kErrSyntax);
  int line = 0;
  CHECK(CompileCode("10 PRINT 1\n20 FOR I=1 TO 3\n", &line) == newbasic::kErrForWithoutNext);
  CHECK(line == 20);
}

TEST_CASE("Statements: runtime errors", "[stmt][errors]") {
  CHECK(RunErrorCode("10 GOTO 999\n") == newbasic::kErrUndefinedLineNumber);
  CHECK(RunErrorCode("10 RETURN\n") == newbasic::kErrReturnWithoutGosub);
  CHECK(RunErrorCode("10 NEXT I\n") == newbasic::kErrNextWithoutFor);
  CHECK(RunErrorCode("10 FOR I=1 TO 3\n20 NEXT J\n") == newbasic::kErrNextWithoutFor);
  int line = 0;
  CHECK(RunErrorCode("10 PRINT 1\n20 GOTO 999\n", &line) == newbasic::kErrUndefinedLineNumber);
  CHECK(line == 20);
}

TEST_CASE("READ fills array elements, in DATA order", "[statements]") {
  // Markers rather than exact spacing, so the assertion is about the values.
  CHECK(lwtest::Row0("10 DIM N(5)\n20 DATA 11, 22\n30 READ N(1), N(2)\n"
                     "40 IF N(1) = 11 THEN PRINT \"A\";\n50 IF N(2) = 22 THEN PRINT \"B\";\n"
                     "60 PRINT\n") == "AB");

  // A string array element takes the next DATA item.
  CHECK(lwtest::Row0("10 DIM S$(5)\n20 DATA \"hi\"\n30 READ S$(2)\n"
                     "40 IF S$(2) = \"hi\" THEN PRINT \"OK\";\n") == "OK");

  // The subscript is an expression, evaluated when READ runs.
  CHECK(lwtest::Row0("10 DIM N(5)\n20 DATA 42\n30 I = 3\n40 READ N(I + 1)\n"
                     "50 IF N(4) = 42 THEN PRINT \"OK\";\n") == "OK");

  // Scalars and array elements can be mixed, and order is preserved.
  CHECK(lwtest::Row0("10 DIM N(5)\n20 DATA 1, 2, 3\n30 READ A, N(2), B\n"
                     "40 IF A = 1 AND N(2) = 2 AND B = 3 THEN PRINT \"OK\";\n") == "OK");

  // Running out of DATA, and a subscript outside the array, report the reference's errors.
  CHECK(RunErrorCode("10 DIM N(3)\n20 READ N(1)\n") == newbasic::kErrOutOfData);
  CHECK(RunErrorCode("10 DIM N(3)\n20 DATA 1\n30 READ N(9)\n") ==
        newbasic::kErrSubscriptOutOfRange);
}

// PRINT USING: one assertion per field kind. The `;` on the PRINT and the "|" on the next
// line make the exact width visible, which is the whole point of a USING field.
static std::string Using(const char* format, const char* item) {
  std::string program = "10 PRINT USING ";
  program += format;
  program += "; ";
  program += item;
  program += ";\n20 PRINT \"|\"\n";
  return lwtest::Row0(program);
}

TEST_CASE("PRINT USING formats numbers", "[statements]") {
  CHECK(Using("\"###\"", "42") == " 42|");
  CHECK(Using("\"###.##\"", "3.14159") == "  3.14|");      // rounded to the decimals
  CHECK(Using("\"##,###.##\"", "1234.5") == " 1,234.50|"); // grouping commas
  CHECK(Using("\"+###\"", "42") == " +42|");               // leading sign column
  CHECK(Using("\"+###\"", "-42") == " -42|");
  CHECK(Using("\"###-\"", "42") == " 42 |"); // trailing sign column
  CHECK(Using("\"###-\"", "-42") == " 42-|");
  CHECK(Using("\"**###\"", "42") == "***42|");             // asterisk fill
  CHECK(Using("\"$$###\"", "42") == "  $42|");             // dollar sign beside the digits
  CHECK(Using("\"**$###\"", "42") == "***$42|");           // both
  CHECK(Using("\"##.##^^^^\"", "1234.5") == " 1.23E+03|"); // exponent form
}
TEST_CASE("PRINT USING formats strings", "[statements]") {
  CHECK(Using("\"!\"", "\"abc\"") == "a|");       // first character only
  CHECK(Using("\"&\"", "\"abc\"") == "abc|");     // the whole string
  CHECK(Using("\"\\ \\\"", "\"abc\"") == "abc|"); // fixed width, padded
}

TEST_CASE("PRINT USING and TAB edge cases", "[statements]") {
  // A negative value keeps its sign inside the asterisk fill.
  CHECK(lwtest::Row0("10 PRINT USING \"**###\"; -42;\n20 PRINT \"|\"\n") == "**-42|");
  // Exponent form with a negative value uses the sign column.
  CHECK(lwtest::Row0("10 PRINT USING \"##.##^^^^\"; -1234.5;\n20 PRINT \"|\"\n") == "-1.23E+03|");
  // An empty string in a variable-length field prints nothing.
  CHECK(lwtest::Row0("10 PRINT USING \"&\"; \"\";\n20 PRINT \"|\"\n") == "|");
  // TAB(0) has no column, which the reference rejects.
  CHECK(lwtest::RunErrorCode("10 PRINT TAB(0); \"X\"\n") == newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("PRINT USING prints an escaped character literally", "[statements]") {
  // `_` prints the next character even when it is a field character. Two items are needed
  // because the literal after the last field of a cycle prints with the next value.
  CHECK(lwtest::Row0("10 PRINT USING \"###_#\"; 42; 43;\n20 PRINT \"|\"\n") == " 42# 43|");
}

TEST_CASE("PRINT USING cycles the format and reports mismatches", "[statements]") {
  // The format restarts for each item, so both fields print.
  CHECK(lwtest::Row0("10 PRINT USING \"##-\"; 1; 2;\n20 PRINT \"|\"\n") == " 1  2 |");
  // A string in a numeric field is an error, not silent output.
  CHECK(lwtest::RunErrorCode("10 PRINT USING \"###\"; \"x\"\n") == newbasic::kErrTypeMismatch);
}

TEST_CASE("TAB and SPC position the cursor", "[statements]") {
  CHECK(lwtest::Row0("10 PRINT TAB(10); \"X\";\n20 PRINT \"|\"\n") == "         X|");
  CHECK(lwtest::Row0("10 PRINT SPC(3); \"X\";\n20 PRINT \"|\"\n") == "   X|");
  // A tab behind the cursor moves to the next line rather than backwards.
  CHECK(lwtest::Row0("10 PRINT \"abc\"; TAB(2); \"X\";\n20 PRINT \"|\"\n") == "abc");
  // A tab past the line width wraps around, as the reference's modulo does.
  CHECK(lwtest::Row0("10 PRINT TAB(85); \"X\";\n20 PRINT \"|\"\n") == "    X|");
}

TEST_CASE("CLEAR works inside a program", "[statements]") {
  // CLEAR empties variables, arrays and strings but leaves the program running, so execution
  // continues at the next instruction with everything reset.
  CHECK(lwtest::Row0("10 A = 5: DIM B(3): B(1) = 7: C$ = \"x\"\n"
                     "20 CLEAR\n"
                     "30 IF A = 0 AND B(1) = 0 AND C$ = \"\" THEN PRINT \"OK\";\n") == "OK");

  // A cleared array is re-created on use, so a write after CLEAR does not reach the old one.
  CHECK(lwtest::Row0("10 DIM B(3): B(1) = 7\n20 CLEAR\n30 B(1) = 9\n"
                     "40 IF B(1) = 9 THEN PRINT \"OK\";\n") == "OK");

  // The argument form (string space / stack size) is not modelled and says so.
  CHECK(lwtest::RunErrorCode("10 CLEAR ,1000\n") == newbasic::kErrAdvancedFeature);
}
