#include "newbasic/host_path.h"

#include <algorithm>

namespace newbasic {

std::string HostPath(const std::string& path) {
#if defined(_WIN32)
  // Windows accepts both separators, but normalising to the native one keeps
  // diagnostics and the DOS command implementations consistent.
  std::string translated = path;
  std::replace(translated.begin(), translated.end(), '/', '\\');
  return translated;
#else
  // DOS uses backslash; POSIX uses slash. A backslash is legal in a POSIX
  // filename, but in a GW-BASIC program it is overwhelmingly a path separator,
  // so treating it as one is the useful compatibility choice.
  std::string translated = path;
  std::replace(translated.begin(), translated.end(), '\\', '/');
  return translated;
#endif
}

} // namespace newbasic
