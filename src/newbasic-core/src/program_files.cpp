#include "newbasic/program_files.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "newbasic/errors.h"
#include "newbasic/fstok/codec.h"
#include "newbasic/fstok/protect.h"
#include "newbasic/host_path.h"
#include "newbasic/line_store.h"

namespace newbasic {

bool LoadProgramFromFile(const std::string& path, LineStore* store, int* error_code,
                         std::string* error) {
  std::ifstream in(HostPath(path), std::ios::binary);
  if (!in) {
    if (error_code != nullptr)
      *error_code = kErrFileNotFound;
    if (error != nullptr)
      *error = "file not found";
    return false;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string content = buffer.str();

  const unsigned char first_byte = content.empty() ? 0u : static_cast<unsigned char>(content[0]);
  if (first_byte == 0xFE || first_byte == 0xFF) {
    // A protected file is the tokenized format in a 0xFE envelope: unprotect it,
    // then decode like any other tokenized program.
    const std::string tokenized = first_byte == 0xFE ? fstok::UnprotectTokenized(content) : content;
    std::vector<fstok::DecodedLine> decoded;
    std::string decode_error;
    if (!fstok::DecodeTokenized(tokenized, &decoded, &decode_error)) {
      if (error_code != nullptr)
        *error_code = kErrBadFileMode;
      if (error != nullptr)
        *error = decode_error;
      return false;
    }
    LineStore loaded;
    for (const fstok::DecodedLine& line : decoded)
      loaded.Add(line.number, line.source);
    *store = std::move(loaded);
    return true;
  }

  LineStore loaded;
  std::string parse_error;
  if (!LineStore::ParseTextBas(content, &loaded, &parse_error)) {
    if (error_code != nullptr)
      *error_code = kErrDirectStatementInFile;
    if (error != nullptr)
      *error = parse_error;
    return false;
  }
  *store = std::move(loaded);
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
