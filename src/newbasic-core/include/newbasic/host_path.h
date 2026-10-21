#pragma once

#include <string>

namespace newbasic {

// Translates a GW-BASIC/DOS path to the host's path syntax. POSIX maps the DOS
// separator '\\' to '/', and Windows maps '/' to '\\'. Drive letters are
// deliberately left alone and documented as unsupported on POSIX.
std::string HostPath(const std::string& path);

} // namespace newbasic
