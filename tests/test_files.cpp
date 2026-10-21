// M3: program files (plain-text SAVE/LOAD, RUN "file") and SHELL.
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/fstok/encoder.h"
#include "newbasic/fstok/protect.h"
#include "newbasic/host_path.h"
#include "newbasic/repl.h"
#include "newbasic/screen/headless_screen.h"
#include "test_helpers.h"

namespace {

// Unique per process: a leftover file cannot affect this run (plan.md 7b).
#define kTempFile lwtest::TempName("newbasic_tmp_prog.bas")

std::string ReadFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

struct Session {
  newbasic::screen::HeadlessScreen screen;
  newbasic::ScriptedHost host;
  newbasic::Repl repl{screen, &host};
};

} // namespace

TEST_CASE("Program files: SAVE writes plain text and LOAD reads it back", "[files]") {
  Session s;
  std::remove(kTempFile);
  CHECK(s.repl.Process("10 PRINT \"HI\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("20 PRINT 42").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process(std::string("SAVE \"") + kTempFile + "\"").status ==
        newbasic::RunResult::Status::Done);
  // Plain text, CRLF, "<number><tab><source>" (plan section 4).
  CHECK(ReadFile(kTempFile) == "10\tPRINT \"HI\"\r\n20\tPRINT 42\r\n");

  s.repl.Process("NEW");
  CHECK(s.repl.Store().Size() == 0);
  CHECK(s.repl.Process(std::string("LOAD \"") + kTempFile + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 2);
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  const std::vector<std::string> rows = s.screen.RenderRows();
  REQUIRE(rows.size() >= 2);
  CHECK(rows[0] == "HI");
  CHECK(rows[1] == " 42");
  std::remove(kTempFile);
}

TEST_CASE("Program files: format auto-detection", "[files]") {
  Session s;
  std::remove(kTempFile);
  // A leading 0xFE marks a protected program: LOAD unprotects it and runs it.
  {
    const std::vector<newbasic::fstok::DecodedLine> lines = {{10, "PRINT \"PROTECTED\""},
                                                            {20, "END"}};
    const std::string tokenized = newbasic::fstok::EncodeTokenized(lines);
    const std::string protected_bytes = newbasic::fstok::ProtectTokenized(tokenized);
    std::ofstream out(kTempFile, std::ios::binary);
    out.write(protected_bytes.data(), static_cast<std::streamsize>(protected_bytes.size()));
  }
  const newbasic::RunResult protected_file =
      s.repl.Process("LOAD \"" + std::string(kTempFile) + "\"");
  CHECK(protected_file.status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 2);
  s.screen.Clear();
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "PROTECTED");

  // A leading 0xFF is a tokenized program: M4 decodes it into the line store.
  {
    std::ofstream out(kTempFile, std::ios::binary | std::ios::trunc);
    const std::vector<newbasic::fstok::DecodedLine> lines = {{10, "PRINT \"FROM TOKENIZED\""},
                                                            {20, "END"}};
    const std::string bytes = newbasic::fstok::EncodeTokenized(lines);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  }
  const newbasic::RunResult tokenized = s.repl.Process("LOAD \"" + std::string(kTempFile) + "\"");
  CHECK(tokenized.status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 2);
  // The decoded program runs like any other.
  s.screen.Clear();
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "FROM TOKENIZED");

  // SAVE "file",B round-trips through the tokenized format: the file it writes
  // is byte-for-byte the program that was just loaded.
  std::remove(kTempFile);
  s.repl.Process("10 PRINT \"FROM TOKENIZED\"");
  s.repl.Process("20 END");
  CHECK(s.repl.Process("SAVE \"" + std::string(kTempFile) + "\",B").status ==
        newbasic::RunResult::Status::Done);
  {
    std::ifstream in(kTempFile, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string written = buffer.str();
    REQUIRE(!written.empty());
    CHECK(static_cast<unsigned char>(written[0]) == 0xFF);
    const std::vector<newbasic::fstok::DecodedLine> expected = {{10, "PRINT \"FROM TOKENIZED\""},
                                                               {20, "END"}};
    CHECK(written == newbasic::fstok::EncodeTokenized(expected));
  }
  // SAVE "file",P writes the protected form: the same program with a 0xFE
  // envelope and the Kocher transform applied.
  CHECK(s.repl.Process("SAVE \"" + std::string(kTempFile) + "\",P").status ==
        newbasic::RunResult::Status::Done);
  {
    std::ifstream in(kTempFile, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string written = buffer.str();
    REQUIRE(!written.empty());
    CHECK(static_cast<unsigned char>(written[0]) == 0xFE);
    const std::vector<newbasic::fstok::DecodedLine> expected = {{10, "PRINT \"FROM TOKENIZED\""},
                                                               {20, "END"}};
    CHECK(newbasic::fstok::UnprotectTokenized(written) == newbasic::fstok::EncodeTokenized(expected));
  }
  // And the protected file loads back as the same program.
  CHECK(s.repl.Process("NEW").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("LOAD \"" + std::string(kTempFile) + "\"").status ==
        newbasic::RunResult::Status::Done);
  s.screen.Clear();
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "FROM TOKENIZED");
  // And it loads back as the same program.
  CHECK(s.repl.Process("NEW").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("LOAD \"" + std::string(kTempFile) + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 2);
  std::remove(kTempFile);

  // An empty file is a valid (empty) text program.
  {
    std::ofstream out(kTempFile, std::ios::binary | std::ios::trunc);
  }
  CHECK(s.repl.Process("LOAD \"" + std::string(kTempFile) + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(s.repl.Store().Size() == 0);
  std::remove(kTempFile);
}

TEST_CASE("Program files: RUN \"file\" loads and runs it", "[files]") {
  Session s;
  std::remove(kTempFile);
  {
    std::ofstream out(kTempFile, std::ios::binary);
    out << "10 PRINT \"FROM FILE\"\r\n";
  }
  CHECK(s.repl.Process(std::string("RUN \"") + kTempFile + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(s.screen.RenderRows()[0] == "FROM FILE");
  CHECK(s.repl.Store().Size() == 1);
  std::remove(kTempFile);
}

TEST_CASE("Program files: errors", "[files]") {
  Session s;
  CHECK(s.repl.Process("SAVE").code == newbasic::kErrSyntax);
  CHECK(s.repl.Process("LOAD").code == newbasic::kErrSyntax);
  CHECK(s.repl.Process("LOAD \"no_such_file_xyz.bas\"").code == newbasic::kErrFileNotFound);
  CHECK(s.repl.Process("RUN \"no_such_file_xyz.bas\"").code == newbasic::kErrFileNotFound);
}

TEST_CASE("HostPath normalises DOS and host separators", "[files]") {
#if defined(_WIN32)
  CHECK(newbasic::HostPath("samples/accounting.dat") == "samples\\accounting.dat");
  CHECK(newbasic::HostPath("samples\\accounting.dat") == "samples\\accounting.dat");
#else
  CHECK(newbasic::HostPath("samples\\accounting.dat") == "samples/accounting.dat");
  CHECK(newbasic::HostPath("samples/accounting.dat") == "samples/accounting.dat");
  CHECK(newbasic::HostPath("a\\b\\c") == "a/b/c");
#endif
}

TEST_CASE("DOS commands: NAME, KILL, MKDIR, CHDIR, RMDIR, FILES", "[files]") {
  Session s;
  const std::string dir = "newbasic_tmp_dir";
  const std::string file = "newbasic_tmp_dos.txt";
  const std::string renamed = "newbasic_tmp_dos2.txt";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::remove(file.c_str());
  std::remove(renamed.c_str());

  s.repl.Process("10 PRINT 1");
  CHECK(s.repl.Process("SAVE \"" + file + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("NAME \"" + file + "\" AS \"" + renamed + "\"").status ==
        newbasic::RunResult::Status::Done);
  CHECK(std::filesystem::exists(renamed));
  CHECK(s.repl.Process("KILL \"" + renamed + "\"").status == newbasic::RunResult::Status::Done);
  CHECK_FALSE(std::filesystem::exists(renamed));

  CHECK(s.repl.Process("MKDIR \"" + dir + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(std::filesystem::is_directory(dir));
  CHECK(s.repl.Process("RMDIR \"" + dir + "\"").status == newbasic::RunResult::Status::Done);
  CHECK_FALSE(std::filesystem::exists(dir));

  CHECK(s.repl.Process("KILL \"no_such_file_xyz.txt\"").code == newbasic::kErrFileNotFound);
  CHECK(s.repl.Process("CHDIR \"no_such_dir_xyz\"").code == newbasic::kErrPathNotFound);
  CHECK(s.repl.Process("KILL").code == newbasic::kErrBadFileName);

  s.screen.Clear();
  CHECK(s.repl.Process("FILES").status == newbasic::RunResult::Status::Done);
  CHECK_FALSE(s.screen.RenderRows()[0].empty());
}

TEST_CASE("DOS commands preserve path and type semantics", "[files]") {
  Session s;
  const std::string dir = "newbasic_tmp_path_dir";
  const std::string slash_file = "newbasic_tmp_path_dir\\prog.bas";
  const std::string host_file = "newbasic_tmp_path_dir/prog.bas";
  const std::string source = "newbasic_tmp_name_a.txt";
  const std::string target = "newbasic_tmp_name_b.txt";
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::remove(source.c_str());
  std::remove(target.c_str());

  s.repl.Process("10 PRINT 1");
  CHECK(s.repl.Process("MKDIR \"" + dir + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("SAVE \"" + slash_file + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(std::filesystem::exists(host_file));

  // KILL is for files; a directory must not be removed.
  CHECK(s.repl.Process("KILL \"" + dir + "\"").code == newbasic::kErrFileNotFound);
  CHECK(std::filesystem::is_directory(dir));
  CHECK(s.repl.Process("KILL \"" + slash_file + "\"").status == newbasic::RunResult::Status::Done);
  CHECK_FALSE(std::filesystem::exists(host_file));
  CHECK(s.repl.Process("RMDIR \"" + dir + "\"").status == newbasic::RunResult::Status::Done);
  CHECK_FALSE(std::filesystem::exists(dir));

  // NAME must not overwrite an existing destination.
  CHECK(s.repl.Process("SAVE \"" + source + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("SAVE \"" + target + "\"").status == newbasic::RunResult::Status::Done);
  CHECK(s.repl.Process("NAME \"" + source + "\" AS \"" + target + "\"").code ==
        newbasic::kErrPathFileAccess);
  CHECK(std::filesystem::exists(source));
  CHECK(std::filesystem::exists(target));
  std::remove(source.c_str());
  std::remove(target.c_str());
}

TEST_CASE("SHELL runs commands through the host", "[shell]") {
  Session s;
  CHECK(s.repl.Process("SHELL \"dir\"").status == newbasic::RunResult::Status::Done);
  REQUIRE(s.host.shell_commands.size() == 1);
  CHECK(s.host.shell_commands[0] == "dir");

  // SHELL also works as a program statement.
  s.repl.Process("10 SHELL \"echo hi\"");
  CHECK(s.repl.Process("RUN").status == newbasic::RunResult::Status::Done);
  REQUIRE(s.host.shell_commands.size() == 2);
  CHECK(s.host.shell_commands[1] == "echo hi");

  // SHELL with no command starts an interactive interpreter.
  CHECK(s.repl.Process("SHELL").status == newbasic::RunResult::Status::Done);
  REQUIRE(s.host.shell_commands.size() == 3);
  CHECK(s.host.shell_commands[2].empty());

  // A command built at run time.
  s.repl.Process("A$=\"ver\"");
  s.repl.Process("SHELL A$");
  REQUIRE(s.host.shell_commands.size() == 4);
  CHECK(s.host.shell_commands[3] == "ver");
}

TEST_CASE("EOF reports end of file on an open file", "[files]") {
  // EOF is how a GW-BASIC program stops reading before INPUT # raises "Input past end".
  const char* name = lwtest::TempName("newbasic_eof.txt");
  std::string program = "10 OPEN \"" + std::string(name) + "\" FOR OUTPUT AS #1\n";
  program += "20 PRINT #1, \"one\"\n30 PRINT #1, \"two\"\n40 CLOSE #1\n";
  program += "50 OPEN \"" + std::string(name) + "\" FOR INPUT AS #1\n";
  program += "60 IF EOF(1) THEN PRINT \"EARLY\";\n"; // must be false before any read
  program += "70 LINE INPUT #1, A$\n80 LINE INPUT #1, B$\n";
  program += "90 IF EOF(1) THEN PRINT \"LATE\";\n"; // must be true after the last line
  program += "100 CLOSE #1\n";
  CHECK(lwtest::Row0(program) == "LATE");

  // An unopened handle is a bad file number, not a false answer.
  CHECK(lwtest::RunErrorCode("10 IF EOF(7) THEN PRINT \"X\"\n") == newbasic::kErrBadFileNumber);
  std::remove(name);
}

TEST_CASE("Reading past end of file is the error EOF exists to avoid", "[files]") {
  // The reference: a file input statement that reads past the end raises Input past end (62).
  // EOF(n) is how a program checks first.
  const char* name = lwtest::TempName("newbasic_eof2.txt");
  std::string program = "10 OPEN \"" + std::string(name) + "\" FOR OUTPUT AS #1\n";
  program += "20 PRINT #1, \"only\"\n30 CLOSE #1\n";
  program += "40 OPEN \"" + std::string(name) + "\" FOR INPUT AS #1\n";
  program += "50 LINE INPUT #1, A$\n60 LINE INPUT #1, B$\n"; // second read is past the end
  CHECK(lwtest::RunErrorCode(program) == newbasic::kErrInputPastEnd);
  std::remove(name);
}
