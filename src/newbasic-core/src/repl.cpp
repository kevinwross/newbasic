#include "newbasic/repl.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/host_path.h"
#include "newbasic/program_files.h"
#include "newbasic/renum.h"

namespace newbasic {
namespace {

bool TokenLineNumber(const Token& token, int* out) {
  return token.kind == Tok::Number && LineNumberFromNumber(token.number.AsNumber(), out);
}

} // namespace

RunResult Repl::SaveProgramFile(const std::string& path) {
  RunResult result;
  std::ofstream out(HostPath(path), std::ios::binary);
  if (!out) {
    result.status = RunResult::Status::Error;
    result.code = kErrPathNotFound;
    result.message = ErrorMessage(kErrPathNotFound);
    return result;
  }
  out << store_.ToTextBas();
  if (!out) {
    result.status = RunResult::Status::Error;
    result.code = kErrDeviceIOError;
    result.message = ErrorMessage(kErrDeviceIOError);
  }
  return result;
}

RunResult Repl::LoadProgramFile(const std::string& path) {
  RunResult result;
  std::ifstream in(HostPath(path), std::ios::binary);
  if (!in) {
    result.status = RunResult::Status::Error;
    result.code = kErrFileNotFound;
    result.message = ErrorMessage(kErrFileNotFound);
    return result;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string content = buffer.str();
  // Text .BAS is the only supported program-file format. Reject the old
  // tokenized/protected envelopes with a clear error.
  const unsigned char first_byte = content.empty() ? 0u : static_cast<unsigned char>(content[0]);
  if (first_byte == 0xFE || first_byte == 0xFF) {
    result.status = RunResult::Status::Error;
    result.code = kErrBadFileMode;
    result.message = "tokenized program files are not supported";
    return result;
  }
  LineStore loaded;
  std::string error;
  if (!LineStore::ParseTextBas(content, &loaded, &error)) {
    result.status = RunResult::Status::Error;
    result.code = kErrDirectStatementInFile;
    result.message = error;
    return result;
  }
  store_ = std::move(loaded);
  vm_.Reset();
  return result;
}

Repl::Repl(screen::Screen& screen, Host* host) : screen_(screen), host_(host), vm_(screen, host) {
  // Program-editing statements (LIST/DELETE/NEW) act on the stored source lines.
  vm_.SetLineStore(&store_);
}

void Repl::PrintLine(const std::string& text) {
  screen_.WriteAtCursor(screen::DecodeCp437(text));
  screen_.NewLine();
  screen_.PresentIfDirty();
}

void Repl::PrintError(const RunResult& result) {
  if (result.status != RunResult::Status::Error)
    return;
  PrintLine(FormatError(result.code, result.line, result.message));
}

bool Repl::Recompile(CompileError* error) {
  return Compile(store_, &program_, error);
}

RunResult Repl::RunProgram(int start_line) {
  RunResult result;
  CompileError error;
  if (!Compile(store_, &program_, &error)) {
    result.status = RunResult::Status::Error;
    result.line = error.line;
    result.code = error.code;
    result.message = error.message;
    return result;
  }
  vm_.Reset();
  vm_.Load(program_);
  if (start_line != 0) {
    const std::optional<std::pair<int, int>> range = program_.PcRangeForLine(start_line);
    if (!range) {
      result.status = RunResult::Status::Error;
      result.line = start_line;
      result.code = kErrUndefinedLineNumber;
      result.message = ErrorMessage(kErrUndefinedLineNumber);
      return result;
    }
    vm_.SetPc(range->first);
  }
  return vm_.Run();
}

bool Repl::HandleRun(const std::vector<Token>& tokens, RunResult* result) {
  int start_line = 0;
  if (tokens.size() > 1 && tokens[1].kind == Tok::Number) {
    if (!TokenLineNumber(tokens[1], &start_line)) {
      result->status = RunResult::Status::Error;
      result->code = kErrIllegalFunctionCall;
      result->message = "Line number out of range";
      return true;
    }
  }
  *result = RunProgram(start_line);
  ran_program_ = true;
  return true;
}

void Repl::ListRange(int start, int end, bool have_start, bool have_end) {
  for (const SourceLine& line : store_.Lines()) {
    if (have_start && !have_end && line.line_number != start)
      continue;
    if (have_start && have_end && line.line_number < start)
      continue;
    if (have_end && line.line_number > end)
      continue;
    PrintLine(std::to_string(line.line_number) + " " + line.source);
  }
}

bool Repl::HandleList(const std::vector<Token>& tokens, RunResult* result) {
  int start = 0;
  int end = 0;
  bool have_start = false;
  bool have_end = false;
  bool saw_dash = false;
  for (std::size_t i = 1; i < tokens.size(); ++i) {
    const Token& token = tokens[i];
    if (token.kind == Tok::EndOfLine)
      break;
    if (token.kind == Tok::Number) {
      int value = 0;
      if (!TokenLineNumber(token, &value)) {
        result->status = RunResult::Status::Error;
        result->code = kErrIllegalFunctionCall;
        result->message = "Line number out of range";
        return true;
      }
      if (!saw_dash && !have_start) {
        start = value;
        have_start = true;
      } else {
        end = value;
        have_end = true;
      }
      continue;
    }
    if (token.kind == Tok::Op && token.text == "-") {
      saw_dash = true;
      continue;
    }
    break;
  }
  ListRange(start, end, have_start, have_end);
  return true;
}

bool Repl::HandleDelete(const std::vector<Token>& tokens, RunResult* result) {
  int start = 0;
  int end = 0;
  bool have_start = false;
  bool have_end = false;
  bool saw_dash = false;
  for (std::size_t i = 1; i < tokens.size(); ++i) {
    const Token& token = tokens[i];
    if (token.kind == Tok::EndOfLine)
      break;
    if (token.kind == Tok::Number) {
      int value = 0;
      if (!TokenLineNumber(token, &value)) {
        result->status = RunResult::Status::Error;
        result->code = kErrIllegalFunctionCall;
        result->message = "Line number out of range";
        return true;
      }
      if (!saw_dash && !have_start) {
        start = value;
        have_start = true;
      } else {
        end = value;
        have_end = true;
      }
      continue;
    }
    if (token.kind == Tok::Op && token.text == "-") {
      saw_dash = true;
      continue;
    }
    break;
  }
  if (!have_start && !have_end) {
    result->status = RunResult::Status::Error;
    result->code = kErrSyntax;
    result->message = ErrorMessage(kErrSyntax);
    return true;
  }
  if (!have_start)
    start = 0;
  if (!have_end)
    end = kMaxLineNumber;
  std::vector<int> doomed;
  for (const SourceLine& line : store_.Lines()) {
    if (line.line_number >= start && line.line_number <= end)
      doomed.push_back(line.line_number);
  }
  for (int number : doomed)
    store_.Remove(number);
  return true;
}

RunResult Repl::Process(const std::string& line) {
  RunResult result;
  std::vector<Token> tokens;
  std::string lex_error;
  int lex_pos = 0;
  if (!Lexer::Tokenize(line, &tokens, &lex_error, &lex_pos)) {
    result.status = RunResult::Status::Error;
    result.code = kErrSyntax;
    result.message = lex_error;
    return result;
  }
  if (tokens.size() == 1 && tokens[0].kind == Tok::EndOfLine)
    return result; // blank line

  // Numbered line: edit the program.
  if (tokens[0].kind == Tok::Number) {
    int number = 0;
    if (!TokenLineNumber(tokens[0], &number)) {
      result.status = RunResult::Status::Error;
      result.code = kErrSyntax;
      result.message = "Line number out of range";
      return result;
    }
    const std::size_t source_pos =
        tokens.size() > 1 ? static_cast<std::size_t>(tokens[1].pos) : line.size();
    std::string source = source_pos < line.size() ? line.substr(source_pos) : std::string();
    while (!source.empty() && (source.back() == ' ' || source.back() == '\t'))
      source.pop_back();
    if (source.empty()) {
      store_.Remove(number); // a bare line number deletes the line
      return result;
    }
    if (!store_.Add(number, source)) {
      result.status = RunResult::Status::Error;
      result.code = kErrSyntax;
      result.message = "Line number out of range";
    }
    return result;
  }

  if (tokens[0].kind == Tok::Keyword) {
    const std::string keyword = tokens[0].text;
    if (keyword == "SYSTEM") {
      exit_ = true;
      return result;
    }
    if (keyword == "RUN") {
      if (tokens.size() > 1 && tokens[1].kind == Tok::String) {
        const RunResult loaded = LoadProgramFile(tokens[1].str);
        if (loaded.status == RunResult::Status::Error)
          return loaded;
        return RunProgram(0);
      }
      HandleRun(tokens, &result);
      return result;
    }
    if (keyword == "SAVE") {
      if (tokens.size() < 2 || tokens[1].kind != Tok::String) {
        result.status = RunResult::Status::Error;
        result.code = kErrSyntax;
        result.message = "Expected a file name";
        return result;
      }
      // SAVE is text-only. The old ,B (tokenized) and ,P (protected)
      // modifiers are explicitly rejected instead of being silently ignored.
      for (std::size_t i = 2; i + 1 < tokens.size(); ++i) {
        if (tokens[i].kind != Tok::Op || tokens[i].text != ",")
          continue;
        const std::string& modifier = tokens[i + 1].text;
        if (modifier == "B" || modifier == "b" || modifier == "P" || modifier == "p") {
          result.status = RunResult::Status::Error;
          result.code = kErrBadFileMode;
          result.message = "tokenized program files are not supported";
          return result;
        }
      }
      return SaveProgramFile(tokens[1].str);
    }
    if (keyword == "LOAD") {
      if (tokens.size() < 2 || tokens[1].kind != Tok::String) {
        result.status = RunResult::Status::Error;
        result.code = kErrSyntax;
        result.message = "Expected a file name";
        return result;
      }
      return LoadProgramFile(tokens[1].str);
    }
    // DOS-style commands (M3): FILES/KILL/NAME/MKDIR/CHDIR/RMDIR.
    if (keyword == "FILES" || keyword == "KILL" || keyword == "NAME" || keyword == "MKDIR" ||
        keyword == "CHDIR" || keyword == "RMDIR") {
      auto dos_error = [&](int code) {
        result.status = RunResult::Status::Error;
        result.code = code;
        result.message = ErrorMessage(code);
        return result;
      };
      auto argument = [&](std::size_t index) {
        return index < tokens.size() && tokens[index].kind == Tok::String ? tokens[index].str
                                                                          : std::string();
      };
      std::error_code ec;
      if (keyword == "FILES") {
        // Lists the working directory (the guide's optional pattern is not
        // implemented yet), sorted so the output does not depend on the host
        // filesystem's iteration order.
        std::error_code list_error;
        std::vector<std::string> names;
        for (const auto& entry :
             std::filesystem::directory_iterator(std::filesystem::current_path(), list_error)) {
          names.push_back(entry.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        for (const std::string& name : names)
          PrintLine(name);
        return result;
      }
      const std::string first = HostPath(argument(1));
      if (first.empty())
        return dos_error(kErrBadFileName);
      if (keyword == "KILL") {
        // DOS KILL removes files only. std::filesystem::remove would also
        // remove an empty directory.
        if (!std::filesystem::is_regular_file(first, ec) || ec)
          return dos_error(kErrFileNotFound);
        if (!std::filesystem::remove(first, ec) || ec)
          return dos_error(kErrFileNotFound);
        return result;
      }
      if (keyword == "MKDIR") {
        if (!std::filesystem::create_directory(first, ec) || ec)
          return dos_error(kErrPathNotFound);
        return result;
      }
      if (keyword == "RMDIR") {
        // DOS RMDIR removes an empty directory, not a file.
        if (!std::filesystem::is_directory(first, ec) || ec)
          return dos_error(kErrPathNotFound);
        if (!std::filesystem::remove(first, ec) || ec)
          return dos_error(kErrPathNotFound);
        return result;
      }
      if (keyword == "CHDIR") {
        std::filesystem::current_path(first, ec);
        if (ec)
          return dos_error(kErrPathNotFound);
        return result;
      }
      // NAME <old> AS <new>. DOS refuses to overwrite an existing target;
      // POSIX rename would silently replace it, so check first.
      const std::string second = HostPath(argument(3));
      if (second.empty())
        return dos_error(kErrSyntax);
      std::error_code exists_error;
      if (std::filesystem::exists(second, exists_error) && !exists_error)
        return dos_error(kErrPathFileAccess);
      std::filesystem::rename(first, second, ec);
      if (ec)
        return dos_error(kErrPathFileAccess);
      return result;
    }
    if (keyword == "LIST") {
      HandleList(tokens, &result);
      return result;
    }
    if (keyword == "RENUM") {
      // RENUM [[new][,[old][,increment]]]: defaults 10, first line, 10. The argument
      // slots are positional, so "RENUM 300,,50" sets new and increment.
      int slots[3] = {-1, -1, -1};
      int slot = 0;
      for (std::size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].kind == Tok::EndOfLine)
          break;
        if (tokens[i].kind == Tok::Number) {
          int value = 0;
          if (!TokenLineNumber(tokens[i], &value)) {
            result.status = RunResult::Status::Error;
            result.code = kErrIllegalFunctionCall;
            result.message = "Line number out of range";
            return result;
          }
          if (slot < 3)
            slots[slot] = value;
        } else if (tokens[i].kind == Tok::Op && tokens[i].text == ",") {
          if (slot < 3)
            ++slot;
        } else {
          result.status = RunResult::Status::Error;
          result.code = kErrSyntax;
          result.message = "Expected RENUM [new][,[old][,increment]]";
          return result;
        }
      }
      const int new_start = slots[0] < 0 ? 10 : slots[0];
      const int old_start = slots[1] < 0 ? 0 : slots[1];
      const int increment = slots[2] < 0 ? 10 : slots[2];
      const RenumOutcome renumbered = RenumberProgram(&store_, new_start, old_start, increment);
      if (!renumbered.ok) {
        result.status = RunResult::Status::Error;
        result.code = renumbered.error_code;
        result.message = renumbered.message;
        return result;
      }
      if (renumbered.warned) {
        result.status = RunResult::Status::Error;
        result.code = kErrUndefinedLineNumber;
        result.message = renumbered.message;
        return result;
      }
      CompileError compile_error;
      Program recompiled;
      if (Compile(store_, &recompiled, &compile_error)) {
        program_ = std::move(recompiled);
        vm_.Load(program_);
        vm_.SetLineStore(&store_);
      }
      return result;
    }
    if (keyword == "MERGE") {
      // MERGE "file": add the file's lines to the current program, replacing lines
      // with the same numbers.
      if (tokens.size() < 2 || tokens[1].kind != Tok::String) {
        result.status = RunResult::Status::Error;
        result.code = kErrSyntax;
        result.message = "Expected a file name";
        return result;
      }
      LineStore incoming;
      int error_code = 0;
      std::string error;
      if (!LoadProgramFromFile(tokens[1].str, &incoming, &error_code, &error)) {
        result.status = RunResult::Status::Error;
        result.code = error_code;
        result.message = error;
        return result;
      }
      for (const SourceLine& merged : incoming.Lines())
        store_.Add(merged.line_number, merged.source);
      CompileError compile_error;
      Program recompiled;
      if (!Compile(store_, &recompiled, &compile_error)) {
        result.status = RunResult::Status::Error;
        result.code = compile_error.code;
        result.message = "the merged program does not compile";
        return result;
      }
      program_ = std::move(recompiled);
      vm_.Load(program_);
      vm_.SetLineStore(&store_);
      return result;
    }
    if (keyword == "AUTO") {
      // AUTO [start][,increment]: enter numbered lines until an empty line, or a
      // line that carries its own number, ends the sequence.
      int slots[2] = {-1, -1};
      int slot = 0;
      for (std::size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].kind == Tok::EndOfLine)
          break;
        if (tokens[i].kind == Tok::Number) {
          int value = 0;
          if (!TokenLineNumber(tokens[i], &value)) {
            result.status = RunResult::Status::Error;
            result.code = kErrIllegalFunctionCall;
            result.message = "Line number out of range";
            return result;
          }
          if (slot < 2)
            slots[slot] = value;
        } else if (tokens[i].kind == Tok::Op && tokens[i].text == ",") {
          if (slot < 2)
            ++slot;
        } else {
          result.status = RunResult::Status::Error;
          result.code = kErrSyntax;
          result.message = "Expected AUTO [start][,increment]";
          return result;
        }
      }
      const std::int64_t start =
          slots[0] >= 0
              ? slots[0]
              : (store_.Lines().empty() ? 10
                                        : static_cast<std::int64_t>(store_.Lines().back().line_number) +
                                              10);
      if (start > kMaxLineNumber) {
        result.status = RunResult::Status::Error;
        result.code = kErrIllegalFunctionCall;
        result.message = "Line number out of range";
        return result;
      }
      const int increment = slots[1] > 0 ? slots[1] : 10;
      std::int64_t next = start;
      bool changed = false;
      for (;;) {
        if (next > kMaxLineNumber) {
          result.status = RunResult::Status::Error;
          result.code = kErrIllegalFunctionCall;
          result.message = "Line number out of range";
          break;
        }
        // The generated number is a prompt, not a completed line: keep the caret
        // on it so the user's first character appears immediately after the number.
        screen_.WriteAtCursor(screen::DecodeCp437(std::to_string(next) + " "));
        screen_.PresentNow(); // the prompt must be on screen before blocking
        const std::wstring wide = screen_.ReadLine();
        if (wide.empty())
          break; // an empty line ends the sequence
        const std::string text = screen::EncodeCp437(wide);
        // A line that starts with a number ends AUTO and is entered as typed.
        if (std::isdigit(static_cast<unsigned char>(text[0])) != 0) {
          Process(text);
          break;
        }
        store_.Add(static_cast<int>(next), text);
        changed = true;
        next += increment;
      }
      if (changed) {
        CompileError compile_error;
        Program recompiled;
        if (Compile(store_, &recompiled, &compile_error)) {
          program_ = std::move(recompiled);
          vm_.Load(program_);
          vm_.SetLineStore(&store_);
        }
      }
      return result;
    }
    if (keyword == "DELETE") {
      HandleDelete(tokens, &result);
      return result;
    }
    if (keyword == "NEW") {
      store_.Clear();
      program_ = Program{};
      vm_.Reset();
      return result;
    }
    if (keyword == "CLEAR") {
      vm_.Reset();
      return result;
    }
    if (keyword == "CONT") {
      result.status = RunResult::Status::Error;
      result.code = kErrCantContinue;
      result.message = ErrorMessage(kErrCantContinue);
      return result;
    }
    if (keyword == "TRON" || keyword == "TROFF") {
      result.status = RunResult::Status::Error;
      result.code = kErrAdvancedFeature;
      result.message = keyword + " is not implemented in newbasic yet";
      return result;
    }
  }

  // Direct statement: compile and run in the same VM (variables persist).
  Program program;
  CompileError error;
  if (!CompileDirect(line, &program, &error)) {
    result.status = RunResult::Status::Error;
    result.line = error.line;
    result.code = error.code;
    result.message = error.message;
    return result;
  }
  vm_.Load(std::move(program));
  return vm_.Run();
}

bool Repl::Step() {
  // The readiness marker is not written before every line. It appears once when the session
  // starts and again after a program has run, which is what it actually means.
  if (first_step_) {
    first_step_ = false;
    screen_.WriteAtCursor(L"Ok");
    screen_.NewLine();
  }
  // Force the frame: the draw throttle must not skip a prompt, or the console cursor and the
  // echoed input end up on the wrong line.
  screen_.PresentNow();
  const std::wstring wide = screen_.ReadLine();
  if (wide.empty() && screen_.InputExhausted())
    return false;
  const RunResult result = Process(screen::EncodeCp437(wide));
  PrintError(result);
  // The break message comes before the marker, the way the reference orders them.
  if (result.status == RunResult::Status::Break && result.line > 0) {
    PrintLine("Break in " + std::to_string(result.line));
  }
  if (ran_program_) {
    // Ok always starts on a fresh line, the way the reference does it, so a program that
    // ended mid-line does not get its marker glued to the last text it printed.
    if (screen_.CursorCol() != 0)
      screen_.NewLine();
    ran_program_ = false;
    screen_.WriteAtCursor(L"Ok");
    screen_.NewLine();
  }
  return !exit_;
}
} // namespace newbasic
