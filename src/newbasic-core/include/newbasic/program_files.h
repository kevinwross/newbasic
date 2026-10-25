#pragma once

#include <string>

namespace newbasic {

class LineStore;

// Program-file I/O, shared by the REPL (direct-mode LOAD/SAVE), the CLI, the
// VM (in-program LOAD/SAVE/CHAIN), and the DAP launcher. Only plain-text .BAS
// is supported. A leading 0xFF/0xFE is rejected as an unsupported
// tokenized/protected file. INCLUDE directives are expanded while loading.
//
// All return true on success; on failure they set *error_code to a GW-BASIC
// error number and *error to a message.

bool LoadProgramFromFile(const std::string& path, LineStore* store, int* error_code,
                         std::string* error);

// Parses program text and expands INCLUDE directives relative to base_dir. Used
// for buffers that have no backing file, such as the DAP programText launch.
bool LoadProgramFromText(const std::string& text, const std::string& base_dir,
                         LineStore* store, int* error_code, std::string* error);

// Writes the store as text (the project default for SAVE; see plan.md).
bool SaveProgramToFile(const std::string& path, const LineStore& store, int* error_code,
                       std::string* error);

} // namespace newbasic
