// M0 end-to-end: text .BAS -> line store -> compile -> VM on the headless
// screen (deterministic grid capture). Golden fixtures under fixtures/m0
// are hand-verified (plan section 6.4).
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "lwbasic/frontend.h"
#include "lwbasic/line_store.h"
#include "lwbasic/screen/headless_screen.h"
#include "lwbasic/vm.h"

namespace {

std::string ReadAll(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : text) {
    if (c == '\n') {
      out.push_back(cur);
      cur.clear();
    } else if (c != '\r') {
      cur += c;
    }
  }
  out.push_back(cur);
  return out;
}

// Compares a rendered grid against expected rows (missing rows are blank).
bool GridMatches(const std::string& render, const std::vector<std::string>& expected) {
  const std::vector<std::string> rows = SplitLines(render);
  if (rows.size() != 25)
    return false;
  for (std::size_t i = 0; i < 25; ++i) {
    const std::string& want = i < expected.size() ? expected[i] : std::string();
    if (rows[i] != want)
      return false;
  }
  return true;
}

bool GridMatches(const std::string& render, std::initializer_list<const char*> expected) {
  std::vector<std::string> rows;
  rows.reserve(expected.size());
  for (const char* row : expected)
    rows.emplace_back(row);
  return GridMatches(render, rows);
}

// A headless screen that hands out scripted keys, so a corpus fixture can carry a key sequence
// beside its program (a .keys file) for INKEY$-driven menus and forms.
class KeyedScreen : public lwbasic::screen::HeadlessScreen {
public:
  explicit KeyedScreen(std::wstring keys) : keys_(std::move(keys)) {}
  bool TryGetKey(wchar_t* key) override {
    if (next_ >= keys_.size())
      return false;
    *key = keys_[next_++];
    return true;
  }

private:
  std::wstring keys_;
  std::size_t next_ = 0;
};

// A .keys file scripts the input: each line break is Enter, so one line of the file is one line
// typed. Any other byte is that character.
std::wstring KeysFromText(const std::string& text) {
  std::wstring keys;
  for (const char c : text) {
    if (c == '\n')
      keys.push_back(L'\r');
    else if (c != '\r')
      keys.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
  }
  return keys;
}

// The same runner, with scripted keys. A program that runs out of keys sees TryGetKey return
// false, which is what a closed input stream looks like.
std::string RunSourceWithKeys(const std::string& source, const std::string& keys_text) {
  lwbasic::LineStore store;
  std::string error;
  if (!lwbasic::LineStore::ParseTextBas(source, &store, &error)) {
    FAIL("Parse failed: " << error);
  }
  lwbasic::Program program;
  lwbasic::CompileError cerr;
  if (!lwbasic::Compile(store, &program, &cerr)) {
    FAIL("Compile failed (line " << cerr.line << "): " << cerr.message);
  }
  KeyedScreen screen(KeysFromText(keys_text));
  lwbasic::Vm vm(screen);
  vm.Load(std::move(program));
  const lwbasic::RunResult result = vm.Run();
  if (result.status != lwbasic::RunResult::Status::Done) {
    FAIL("Run failed: " << result.message);
  }
  return screen.Render();
}

// Compiles source text and runs it on a fresh headless screen.
std::string RunSource(const std::string& source) {
  lwbasic::LineStore store;
  std::string error;
  if (!lwbasic::LineStore::ParseTextBas(source, &store, &error)) {
    FAIL("Parse failed: " << error);
  }
  lwbasic::Program program;
  lwbasic::CompileError cerr;
  if (!lwbasic::Compile(store, &program, &cerr)) {
    FAIL("Compile failed (line " << cerr.line << "): " << cerr.message);
  }
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::Vm vm(screen);
  vm.Load(std::move(program));
  const lwbasic::RunResult result = vm.Run();
  if (result.status != lwbasic::RunResult::Status::Done) {
    FAIL("Run failed: " << result.message);
  }
  return screen.Render();
}

// Returns the parse/compile error for source that is expected to fail, or
// an empty string when it compiles.
std::string CompileErrorFor(const std::string& source) {
  lwbasic::LineStore store;
  std::string error;
  if (!lwbasic::LineStore::ParseTextBas(source, &store, &error)) {
    return error;
  }
  lwbasic::Program program;
  lwbasic::CompileError cerr;
  if (!lwbasic::Compile(store, &program, &cerr)) {
    return cerr.message;
  }
  return "";
}

} // namespace

TEST_CASE("VM: PRINT string", "[vm][golden]") {
  CHECK(GridMatches(RunSource("10 PRINT \"HELLO, WORLD\"\n"), {"HELLO, WORLD"}));
}

TEST_CASE("VM: PRINT comma zones (14 columns)", "[vm][golden]") {
  // " 5" occupies cols 0-1 with its trailing space at col 2; the comma then
  // advances to the next 14-column zone, so " 10" starts at col 14 (with its
  // own leading space) and the trimmed row ends at col 16.
  const std::string row = " 5" + std::string(13, ' ') + "10";
  CHECK(GridMatches(RunSource("10 PRINT 5, 10\n"), std::vector<std::string>{row}));
}

TEST_CASE("VM: PRINT semicolon", "[vm][golden]") {
  // Each number carries its own sign space and trailing space, so -5 and
  // 2.5 print as "-5" + ' ' then " 2.5" + ' '.
  CHECK(GridMatches(RunSource("10 PRINT -5; 2.5\n"), {"-5  2.5"}));
}

TEST_CASE("VM: trailing semicolon continues the line", "[vm][golden]") {
  CHECK(GridMatches(RunSource("10 PRINT 1;\n20 PRINT 2\n"), {" 1  2"}));
}

TEST_CASE("VM: trailing comma continues at next zone", "[vm][golden]") {
  // After " 1" + its trailing space (cols 0-2), the comma advances to zone
  // col 14, where the next PRINT writes " 2".
  const std::string row = " 1" + std::string(13, ' ') + "2";
  CHECK(GridMatches(RunSource("10 PRINT 1,\n20 PRINT 2\n"), std::vector<std::string>{row}));
}

TEST_CASE("VM: CLS and LOCATE", "[vm][golden]") {
  // LOCATE 2,10 is 1-based: grid row 1, col 9 ("AB" was cleared by CLS).
  const std::string row = std::string(9, ' ') + "XY";
  CHECK(GridMatches(RunSource("10 PRINT \"AB\"\n20 CLS\n30 LOCATE 2, 10\n"
                              "40 PRINT \"XY\"\n"),
                    std::vector<std::string>{"", row}));
}

TEST_CASE("VM: embedded quotes in strings", "[vm][golden]") {
  CHECK(GridMatches(RunSource("10 PRINT \"A\"\"B\"\n"), {"A\"B"}));
}

TEST_CASE("VM: 80-character print wraps, then line end skips a row", "[vm][golden]") {
  const std::string a80(80, 'A');
  CHECK(
      GridMatches(RunSource("10 PRINT \"" + a80 + "\"\n20 PRINT \"B\"\n"), {a80.c_str(), "", "B"}));
}

TEST_CASE("VM: literal kinds", "[vm][golden]") {
  // 345692811 is 9 digits -> Double; &H7F is Integer 127; 1.25 is Single.
  CHECK(GridMatches(RunSource("10 PRINT 345692811\n20 PRINT &H7F; 1.25\n"),
                    {" 345692811", " 127  1.25"}));
}

TEST_CASE("Compile: unknown statements are rejected", "[vm]") {
  CHECK(!CompileErrorFor("10 FOO 1\n").empty());
  CHECK(CompileErrorFor("10 PRINT 1\n").empty());
  CHECK(CompileErrorFor("10 PRINT \"unterminated\n").find("Unterminated") != std::string::npos);
}

TEST_CASE("Golden fixture: fixtures/m0/hello.bas", "[golden]") {
  const std::string source = ReadAll(std::string(LWBASIC_FIXTURES_DIR) + "/m0/hello.bas");
  const std::string expected = ReadAll(std::string(LWBASIC_FIXTURES_DIR) + "/m0/hello.expected");
  CHECK(GridMatches(RunSource(source), SplitLines(expected)));
}

// ---------------------------------------------------------------- M6 corpus
#include <algorithm>
#include <filesystem>

TEST_CASE("Golden corpus: every fixtures/**/*.bas matches its .expected", "[golden][corpus]") {
  namespace fs = std::filesystem;
  const fs::path root(LWBASIC_FIXTURES_DIR);
  REQUIRE(fs::exists(root));

  std::vector<fs::path> programs;
  for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".bas") {
      programs.push_back(entry.path());
    }
  }
  std::sort(programs.begin(), programs.end());
  REQUIRE(!programs.empty());

  for (const fs::path& program : programs) {
    const std::string program_path = program.string();
    const std::string expected_path = program_path.substr(0, program_path.size() - 4) + ".expected";
    INFO("corpus fixture: " << program.filename().string());
    REQUIRE(fs::exists(expected_path));
    // An optional .keys file gives the program a scripted input stream.
    const std::string keys_path = program_path.substr(0, program_path.size() - 4) + ".keys";
    const std::string rendered = fs::exists(keys_path)
                                     ? RunSourceWithKeys(ReadAll(program_path), ReadAll(keys_path))
                                     : RunSource(ReadAll(program_path));
    CHECK(GridMatches(rendered, SplitLines(ReadAll(expected_path))));
  }
}

TEST_CASE("Conformance: documented behaviours across statements", "[golden][conformance]") {
  // Spec-driven cases: each pins a documented behaviour rather than an
  // implementation detail, so a regression in the interpreter or the numeric
  // formatting shows up as a diff against expected screen text.
  struct Case {
    const char* source;
    std::vector<std::string> rows;
  };
  const std::vector<Case> cases = {
      {"10 PRINT 1+2\n", {" 3"}},
      {"10 PRINT 2*3+4\n", {" 10"}}, // multiplication first
      {"10 PRINT 10-2-3\n", {" 5"}}, // left associative
      {"10 PRINT 7\\2\n", {" 3"}},   // integer division
      {"10 PRINT 7 MOD 3\n", {" 1"}},
      {"10 PRINT 2^10\n", {" 1024"}},
      {"10 PRINT \"A\"+\"B\"\n", {"AB"}}, // string concatenation
      {"10 PRINT LEFT$(\"HELLO\",2)\n", {"HE"}},
      {"10 PRINT RIGHT$(\"HELLO\",2)\n", {"LO"}},
      {"10 PRINT MID$(\"HELLO\",2,3)\n", {"ELL"}},
      {"10 PRINT LEN(\"HELLO\")\n", {" 5"}},
      {"10 PRINT ASC(\"A\")\n", {" 65"}},
      {"10 PRINT CHR$(66)\n", {"B"}},
      {"10 PRINT VAL(\"12\")+1\n", {" 13"}},
      {"10 PRINT STR$(42)\n", {" 42"}},
      {"10 PRINT 1/4\n", {" .25"}},
      {"10 PRINT 1=1\n", {"-1"}}, // true is -1 in the dialect
      {"10 PRINT 1=2\n", {" 0"}},
      {"10 IF 1 THEN PRINT \"Y\" ELSE PRINT \"N\"\n", {"Y"}},
      {"10 A=0\n20 FOR I=5 TO 1 STEP -1: A=A+1: NEXT I\n30 PRINT A\n", {" 5"}},
      {"10 A=1\n20 WHILE A<3: A=A+1: WEND\n30 PRINT A\n", {" 3"}},
      {"10 DIM A(2)\n20 A(1)=7\n30 PRINT A(1)\n", {" 7"}},
      {"10 DATA 4,5\n20 READ X,Y\n30 PRINT X+Y\n", {" 9"}},
      {"10 DEF FNX(X)=X*X\n20 PRINT FNX(7)\n", {" 49"}},
      {"10 ON ERROR GOTO 100\n20 PRINT 1/0\n30 END\n100 PRINT \"CAUGHT\"\n110 RESUME NEXT\n",
       {"CAUGHT"}},
  };

  for (const Case& test_case : cases) {
    INFO("conformance source: " << test_case.source);
    CHECK(GridMatches(RunSource(test_case.source), test_case.rows));
  }
}

TEST_CASE("Conformance: numeric display precision", "[golden][conformance]") {
  // The display rules are the behaviour-densest part of the dialect: single
  // precision shows 7 significant digits, double 16, integers print without a
  // fractional part, and values below 1 have no leading zero. These pin digit
  // counts and rounding; the exponent-notation boundaries are separate.
  struct Case {
    const char* source;
    std::vector<std::string> rows;
  };
  const std::vector<Case> cases = {
      {"10 PRINT 1/3\n", {" .3333333"}}, // 7 significant digits
      {"10 PRINT 2/3\n", {" .6666667"}}, // rounded, not truncated
      {"10 PRINT 100/3\n", {" 33.33333"}},
      {"10 PRINT 1/4\n", {" .25"}}, // no leading zero
      {"10 PRINT SQR(2)\n", {" 1.414214"}},
      {"10 PRINT 1000000\n", {" 1000000"}}, // 7-digit integer stays integer
      {"10 PRINT 1234567\n", {" 1234567"}},
      {"10 PRINT 12345678\n", {" 12345678"}}, // 8 digits promotes to double
      {"10 PRINT 0.5*0.5\n", {" .25"}},
      {"10 PRINT 10/4\n", {" 2.5"}},
      {"10 PRINT 1E3\n", {" 1000"}}, // integral value drops the exponent
  };
  for (const Case& test_case : cases) {
    INFO("display source: " << test_case.source);
    CHECK(GridMatches(RunSource(test_case.source), test_case.rows));
  }
}

TEST_CASE("Conformance: PRINT separators and the width boundary", "[golden][conformance]") {
  // From the reference's PRINT section: numbers are always followed by a space,
  // positive numbers are preceded by one, negative numbers by a minus sign, and
  // a carriage return is inserted automatically after `width` characters (80 by
  // default) - which skips two lines when exactly 80 are printed unless the
  // statement ends in a semicolon.
  const std::string eighty(80, 'X');
  struct Case {
    const char* source;
    std::vector<std::string> rows;
  };
  const std::vector<Case> cases = {
      // A number carries its own leading/!!trailing!! spaces.
      {"10 PRINT 42;\n20 PRINT \"X\"\n", {" 42 X"}},
      {"10 PRINT 1;2\n", {" 1  2"}},
      {"10 PRINT -1.5\n", {"-1.5"}},
      {"10 PRINT 1;-2\n", {" 1 -2"}},
      // Exactly 80 characters without a trailing semicolon skips two lines.
      {"10 PRINT STRING$(80,\"X\")\n20 PRINT \"NEXT\"\n", {eighty, "", "NEXT"}},
      // With the semicolon only the automatic width wrap happens.
      {"10 PRINT STRING$(80,\"X\");\n20 PRINT \"NEXT\"\n", {eighty, "NEXT"}},
      // Over the width, the remainder continues on the next row.
      {"10 PRINT STRING$(90,\"Y\")\n", {std::string(80, 'Y'), std::string(10, 'Y')}},
  };
  for (const Case& test_case : cases) {
    INFO("PRINT source: " << test_case.source);
    CHECK(GridMatches(RunSource(test_case.source), test_case.rows));
  }
}
