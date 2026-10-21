#pragma once

#include <string>

namespace lwbasic {

// Translates a GW-BASIC/DOS path to the host's path syntax. Windows returns
// the string unchanged; POSIX maps the DOS separator '\\' to '/'. Drive
// letters are deliberately left alone and documented as unsupported.
std::string HostPath(const std::string& path);

} // namespace lwbasic
