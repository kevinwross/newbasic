#pragma once

#include <string>

namespace lwbasic {

class LineStore;

// Program-file I/O, shared by the REPL (direct-mode LOAD/SAVE) and the VM
// (in-program LOAD/SAVE/CHAIN, M10). Reading auto-detects the format: a leading
// 0xFF is tokenized, a leading 0xFE is a protected tokenized file, anything else
// is text (see docs/m4-tokenized-format-notes.md).
//
// Both return true on success; on failure they set *error_code to a GW-BASIC error
// number and *error to a message.

bool LoadProgramFromFile(const std::string& path, LineStore* store, int* error_code,
                         std::string* error);

// Writes the store as text (the project default for SAVE; see plan.md).
bool SaveProgramToFile(const std::string& path, const LineStore& store, int* error_code,
                       std::string* error);

} // namespace lwbasic
