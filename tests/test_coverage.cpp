// Coverage batch: features that were untested or thinly tested, with
// expectations taken from the reference (docs/gw-basic) rather than from
// whatever the implementation happened to do.
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

// PRINT TAB(n): moves to column n (1-based); if n is at or behind the current
// position the next line is used.
TEST_CASE("Coverage: PRINT TAB positions the cursor", "[coverage]") {
  CHECK(lwtest::Row0("10 PRINT TAB(10);\"X\"\n") == "         X");
  // Already past the requested column: the reference moves to the next line.
  CHECK(lwtest::Row0("10 PRINT \"ABC\" TAB(2);\"X\"\n") == "ABC");
  const std::vector<std::string> rows = lwtest::RunRows("10 PRINT \"ABC\" TAB(2);\"X\"\n");
  REQUIRE(rows.size() >= 2);
  CHECK(rows[1] == " X");
}

// String function edges: indexes past the end, lengths past the end.
TEST_CASE("Coverage: string function edge cases", "[coverage]") {
  // MID$ with a start past the end yields an empty string.
  CHECK(lwtest::Row0("10 PRINT MID$(\"HELLO\",10)\n") == "");
  // A length running past the end is clipped, not an error.
  CHECK(lwtest::Row0("10 PRINT MID$(\"HELLO\",2,100)\n") == "ELLO");
  CHECK(lwtest::Row0("10 PRINT MID$(\"HELLO\",2)\n") == "ELLO");
  // LEFT$/RIGHT$ past the length return the whole string.
  CHECK(lwtest::Row0("10 PRINT LEFT$(\"AB\",5)\n") == "AB");
  CHECK(lwtest::Row0("10 PRINT RIGHT$(\"AB\",5)\n") == "AB");
  // A zero or negative length is empty.
  CHECK(lwtest::Row0("10 PRINT LEFT$(\"AB\",0)\n") == "");
  // LEN of the empty string.
  CHECK(lwtest::Row0("10 PRINT LEN(\"\")\n") == " 0");
}

// STR$ keeps the sign/space convention of PRINT.
TEST_CASE("Coverage: STR$ formatting", "[coverage]") {
  CHECK(lwtest::Row0("10 PRINT STR$(42)\n") == " 42");
  CHECK(lwtest::Row0("10 PRINT STR$(-1.5)\n") == "-1.5");
  // A positive number contributes its leading space, so the length includes it.
  CHECK(lwtest::Row0("10 PRINT LEN(STR$(7))\n") == " 2");
  // STR$ of a value below 1 has no leading zero, like PRINT.
  CHECK(lwtest::Row0("10 PRINT STR$(.25)\n") == " .25");
}

// INPUT and LINE INPUT are covered through the REPL path (tests/test_repl.cpp):
// a direct VM run stops at the prompt, so driving them needs the REPL's input
// loop rather than RunRowsWithInput.

// Numeric typing: the integer suffix rounds to nearest (half away from zero),
// and DEFINT changes the default for names starting with a letter.
TEST_CASE("Coverage: integer type suffixes and DEFINT", "[coverage]") {
  CHECK(lwtest::Row0("10 A%=2.7\n20 PRINT A%\n") == " 3"); // 2.7 rounds to 3
  CHECK(lwtest::Row0("10 A%=2.4\n20 PRINT A%\n") == " 2");
  CHECK(lwtest::Row0("10 A%=-2.7\n20 PRINT A%\n") == "-3");
  CHECK(lwtest::Row0("10 DEFINT A\n20 A=2.7\n30 PRINT A\n") == " 3");
  CHECK(lwtest::Row0("10 A!=1/3\n20 PRINT A!\n") == " .3333333");
}

// ON index GOTO/GOSUB dispatch, including an index past the end (a no-op).
TEST_CASE("Coverage: ON index GOTO and GOSUB", "[coverage]") {
  CHECK(lwtest::Row0("10 ON 2 GOTO 100,200,300\n100 PRINT \"ONE\": END\n"
                     "200 PRINT \"TWO\": END\n300 PRINT \"THREE\": END\n") == "TWO");
  CHECK(lwtest::Row0("10 ON 3 GOSUB 100,200,300\n20 PRINT \"BACK\": END\n"
                     "100 PRINT \"A\": RETURN\n200 PRINT \"B\": RETURN\n"
                     "300 PRINT \"C\": RETURN\n") == "C");
  // An index outside 1..n falls through to the next statement.
  CHECK(lwtest::Row0("10 ON 9 GOTO 100,200\n20 PRINT \"NONE\": END\n"
                     "100 PRINT \"ONE\": END\n200 PRINT \"TWO\": END\n") == "NONE");
}

// INSTR with an explicit start position.
TEST_CASE("Coverage: INSTR with a start argument", "[coverage]") {
  CHECK(lwtest::Row0("10 PRINT INSTR(\"HELLO\",\"L\")\n") == " 3");
  CHECK(lwtest::Row0("10 PRINT INSTR(4,\"HELLO\",\"L\")\n") == " 4");
  CHECK(lwtest::Row0("10 PRINT INSTR(5,\"HELLO\",\"L\")\n") == " 0"); // not found
  CHECK(lwtest::Row0("10 PRINT INSTR(\"HELLO\",\"Z\")\n") == " 0");
}

// RND is in [0, 1) and SWAP exchanges values.
TEST_CASE("Coverage: RND range and SWAP", "[coverage]") {
  CHECK(lwtest::Row0("10 X=RND\n20 PRINT (X>=0 AND X<1)\n") == "-1"); // true
  CHECK(lwtest::Row0("10 A=1: B=2\n20 SWAP A,B\n30 PRINT A;B\n") == " 2  1");
}

// LIST reproduces the program with its line numbers, one line per row.
TEST_CASE("Coverage: LIST output", "[coverage]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT \"HI\"");
  repl.Process("20 END");
  repl.Process("LIST");
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "10 PRINT \"HI\"");
  CHECK(rows[1] == "20 END");
}

// PRINT separators: a comma advances to the next 14-column zone, SPC(n) emits
// n spaces, and the trailing separator suppresses the newline.
TEST_CASE("Coverage: PRINT zones, SPC and trailing separators", "[coverage]") {
  // Items land in zones of 14 columns; assert positions rather than guessing the
  // exact padding, which is the behaviour that matters.
  const std::string zones = lwtest::Row0("10 PRINT 1,2,3\n");
  CHECK(zones.find('1') == 1);  // after the sign space
  CHECK(zones.find('2') == 15); // second zone (1-based column 16)
  CHECK(zones.find('3') == 29); // third zone

  CHECK(lwtest::Row0("10 PRINT SPC(3);\"X\"\n") == "   X");
  CHECK(lwtest::Row0("10 PRINT SPC(0);\"X\"\n") == "X");

  // A trailing semicolon keeps the cursor on the same row, so the next PRINT
  // continues it; a trailing comma does the same after padding to the zone.
  CHECK(lwtest::Row0("10 PRINT \"A\";\n20 PRINT \"B\"\n") == "AB");
  const std::string with_comma = lwtest::Row0("10 PRINT \"A\",\n20 PRINT \"B\"\n");
  CHECK(with_comma.find('A') == 0);
  CHECK(with_comma.find('B') == 14); // padded to the next zone, same row
}

// Screen control: CLS, LOCATE and COLOR act on the grid and the current
// attribute, which the headless screen records for inspection.
TEST_CASE("Coverage: CLS, LOCATE and COLOR", "[coverage]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  repl.Process("10 CLS");
  repl.Process("20 LOCATE 3,5");
  repl.Process("30 PRINT \"X\"");
  repl.Process("RUN");
  const std::vector<std::string> rows = screen.RenderRows();
  REQUIRE(rows.size() > 2);
  // LOCATE 3,5 positions the cursor two rows down, four columns in.
  CHECK(rows[2] == "    X");

  // COLOR changes the current attribute; the next PRINT carries it.
  repl.Process("10 CLS: COLOR 2,4");
  repl.Process("20 PRINT \"Y\"");
  repl.Process("RUN");
  CHECK(screen.CellAt(0, 0) == L'Y');
  CHECK(screen.CellAttrAt(0, 0) == newbasic::screen::Screen::MakeAttr(2, 4));
}

// INKEY$ returns scripted keys, one press per read, and an empty string when
// none is pending.
TEST_CASE("Coverage: INKEY$ consumes scripted keys", "[coverage]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  screen.ScriptKey(L'A');
  screen.ScriptKey(L'b');
  repl.Process("10 K$=INKEY$");
  repl.Process("20 PRINT \"[\";K$;\"]\"");
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == "[A]");

  // A second read gets the next key; a third gets nothing.
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == "[b]");
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == "[]");
}

// INPUT and LINE INPUT read whole lines from the device. This drives them
// through the REPL input loop (a direct VM run stops at the prompt). The prompt
// occupies the first row, so the program's output follows it.
TEST_CASE("Coverage: INPUT and LINE INPUT through the REPL", "[coverage]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl(screen, &host);
  // INPUT splits on commas and converts by target type.
  screen.ScriptLine(L"12,WORLD");
  repl.Process("10 INPUT A,B$");
  repl.Process("20 PRINT A;\"-\";B$");
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.Render().find(" 12 -WORLD") != std::string::npos);

  // LINE INPUT keeps the whole line, commas included.
  screen.ScriptLine(L"a,b c");
  repl.Process("10 LINE INPUT A$");
  repl.Process("20 PRINT \"[\";A$;\"]\"");
  screen.Clear();
  repl.Process("RUN");
  CHECK(screen.Render().find("[a,b c]") != std::string::npos);
}

// RANDOMIZE seeds the generator, so the same seed reproduces the same sequence.
TEST_CASE("Coverage: RANDOMIZE is reproducible", "[coverage]") {
  const auto first_value = [](int seed) {
    newbasic::screen::HeadlessScreen screen;
    newbasic::ScriptedHost host;
    newbasic::Repl repl(screen, &host);
    repl.Process("10 RANDOMIZE " + std::to_string(seed));
    repl.Process("20 PRINT RND");
    repl.Process("RUN");
    return screen.RenderRows()[0];
  };
  CHECK(first_value(42) == first_value(42)); // same seed, same value
  CHECK(first_value(42) != first_value(43)); // different seed, different value
}
