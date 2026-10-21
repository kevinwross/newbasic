#include "lwbasic/host_path.h"

#include <algorithm>

namespace lwbasic {

std::string HostPath(const std::string& path) {
#if defined(_WIN32)
  return path;
#else
  std::string translated = path;
  std::replace(translated.begin(), translated.end(), '\\', '/');
  return translated;
#endif
}

} // namespace lwbasic
