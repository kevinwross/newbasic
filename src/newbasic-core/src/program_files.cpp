#include "newbasic/program_files.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/host_path.h"
#include "newbasic/lexer.h"
#include "newbasic/line_store.h"

namespace newbasic {
namespace {

constexpr int kMaxIncludeDepth = 64;

struct IncludeState {
  int depth = 0;
  std::set<std::string> completed;
  std::vector<std::string> active;
};

bool Fail(int code, std::string message, int* error_code, std::string* error) {
  if (error_code != nullptr)
    *error_code = code;
  if (error != nullptr)
    *error = std::move(message);
  return false;
}

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
  std::error_code error;
  const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
  return error ? path.lexically_normal() : canonical;
}

std::string PathKey(const std::filesystem::path& path) {
  std::string key = NormalizePath(path).string();
#ifdef _WIN32
  for (char& ch : key)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
#endif
  return key;
}

std::string JoinPaths(const std::vector<std::string>& paths) {
  std::string joined;
  for (const std::string& path : paths) {
    if (!joined.empty())
      joined += " -> ";
    joined += path;
  }
  return joined;
}

bool ReadProgramText(const std::filesystem::path& path, std::string* text, int* error_code,
                     std::string* error) {
  std::error_code filesystem_error;
  if (!std::filesystem::is_regular_file(path, filesystem_error)) {
    return Fail(kErrFileNotFound, "file not found: " + path.string(), error_code, error);
  }
  std::ifstream in(HostPath(path.string()), std::ios::binary);
  if (!in)
    return Fail(kErrFileNotFound, "file not found: " + path.string(), error_code, error);
  std::ostringstream buffer;
  buffer << in.rdbuf();
  *text = buffer.str();

  const unsigned char first_byte = text->empty() ? 0u : static_cast<unsigned char>((*text)[0]);
  if (first_byte == 0xFE || first_byte == 0xFF) {
    return Fail(kErrBadFileMode,
                "tokenized program files are not supported: " + path.string(), error_code,
                error);
  }
  return true;
}

enum class IncludeScan { No, Yes, Error };

IncludeScan ScanInclude(const SourceLine& line, std::string* target, std::string* message) {
  std::vector<Token> tokens;
  std::string lex_error;
  int lex_pos = 0;
  if (!Lexer::Tokenize(line.source, &tokens, &lex_error, &lex_pos))
    return IncludeScan::No; // CompileSource reports the lexical error.
  if (tokens.empty() || tokens[0].kind != Tok::Ident || tokens[0].text != "INCLUDE")
    return IncludeScan::No;
  if (tokens.size() < 2 || tokens[1].kind != Tok::String) {
    *message = "INCLUDE needs a quoted file name";
    return IncludeScan::Error;
  }
  if (tokens[1].str.empty()) {
    *message = "INCLUDE needs a non-empty file name";
    return IncludeScan::Error;
  }
  const std::size_t next = 2;
  if (tokens[next].kind != Tok::EndOfLine &&
      !(tokens[next].kind == Tok::Op && tokens[next].text == "'") &&
      !(tokens[next].kind == Tok::Keyword && tokens[next].text == "REM")) {
    *message = "INCLUDE must be the only statement on its line";
    return IncludeScan::Error;
  }
  *target = tokens[1].str;
  return IncludeScan::Yes;
}

std::filesystem::path ResolveInclude(const std::filesystem::path& including_file,
                                     const std::string& target) {
  std::filesystem::path resolved = std::filesystem::path(HostPath(target));
  if (resolved.is_relative())
    resolved = including_file.parent_path() / resolved;
  return NormalizePath(resolved);
}

bool MergeLine(LineStore* out, const SourceLine& line, const std::string& source_file,
               int* error_code, std::string* error) {
  if (out->FindIndex(line.line_number)) {
    return Fail(kErrSyntax,
                "INCLUDE line number conflict at line " + std::to_string(line.line_number) +
                    " while reading " + source_file,
                error_code, error);
  }
  out->Add(line.line_number, line.source);
  return true;
}

bool ProcessStore(const LineStore& source, const std::filesystem::path& source_path,
                  LineStore* out, IncludeState* state, int* error_code, std::string* error);

bool ExpandFile(const std::filesystem::path& path, LineStore* out, IncludeState* state,
                int* error_code, std::string* error) {
  const std::filesystem::path normalized = NormalizePath(path);
  const std::string key = PathKey(normalized);
  if (std::find(state->active.begin(), state->active.end(), key) != state->active.end()) {
    std::vector<std::string> cycle = state->active;
    cycle.push_back(normalized.string());
    return Fail(kErrSyntax, "INCLUDE cycle: " + JoinPaths(cycle), error_code, error);
  }
  if (state->completed.count(key) != 0)
    return true;
  if (state->depth >= kMaxIncludeDepth)
    return Fail(kErrSyntax, "INCLUDE nesting is too deep", error_code, error);

  std::string text;
  if (!ReadProgramText(normalized, &text, error_code, error))
    return false;

  LineStore parsed;
  std::string parse_error;
  if (!LineStore::ParseTextBas(text, &parsed, &parse_error)) {
    return Fail(kErrDirectStatementInFile, normalized.string() + ": " + parse_error,
                error_code, error);
  }

  state->active.push_back(key);
  ++state->depth;
  const bool ok = ProcessStore(parsed, normalized, out, state, error_code, error);
  --state->depth;
  state->active.pop_back();
  if (ok)
    state->completed.insert(key);
  return ok;
}

bool ProcessStore(const LineStore& source, const std::filesystem::path& source_path,
                  LineStore* out, IncludeState* state, int* error_code, std::string* error) {
  for (const SourceLine& line : source.Lines()) {
    std::string target;
    std::string scan_error;
    const IncludeScan scan = ScanInclude(line, &target, &scan_error);
    if (scan == IncludeScan::Error) {
      return Fail(kErrSyntax,
                  source_path.string() + ":" + std::to_string(line.line_number) + ": " +
                      scan_error,
                  error_code, error);
    }
    if (scan == IncludeScan::Yes) {
      // Keep the include line's number as a harmless placeholder so existing
      // GOTO/GOSUB targets and RENUM references remain valid.
      SourceLine placeholder;
      placeholder.line_number = line.line_number;
      placeholder.source = "REM " + line.source;
      if (!MergeLine(out, placeholder, source_path.string(), error_code, error))
        return false;
      if (!ExpandFile(ResolveInclude(source_path, target), out, state, error_code, error))
        return false;
    } else if (!MergeLine(out, line, source_path.string(), error_code, error)) {
      return false;
    }
  }
  return true;
}

} // namespace

bool LoadProgramFromFile(const std::string& path, LineStore* store, int* error_code,
                         std::string* error) {
  LineStore expanded;
  IncludeState state;
  if (!ExpandFile(std::filesystem::path(HostPath(path)), &expanded, &state, error_code, error))
    return false;
  *store = std::move(expanded);
  return true;
}

bool LoadProgramFromText(const std::string& text, const std::string& base_dir,
                         LineStore* store, int* error_code, std::string* error) {
  LineStore parsed;
  std::string parse_error;
  if (!LineStore::ParseTextBas(text, &parsed, &parse_error)) {
    return Fail(kErrDirectStatementInFile, "<program text>: " + parse_error, error_code,
                error);
  }
  const std::filesystem::path source_path =
      std::filesystem::path(HostPath(base_dir)) / "<program text>";
  LineStore expanded;
  IncludeState state;
  if (!ProcessStore(parsed, source_path, &expanded, &state, error_code, error))
    return false;
  *store = std::move(expanded);
  return true;
}

bool SaveProgramToFile(const std::string& path, const LineStore& store, int* error_code,
                       std::string* error) {
  std::ofstream out(HostPath(path), std::ios::binary);
  if (!out) {
    if (error_code != nullptr)
      *error_code = kErrPathNotFound;
    if (error != nullptr)
      *error = "cannot open the file for writing";
    return false;
  }
  out << store.ToTextBas();
  if (!out) {
    if (error_code != nullptr)
      *error_code = kErrDeviceIOError;
    if (error != nullptr)
      *error = "write failed";
    return false;
  }
  return true;
}

} // namespace newbasic
