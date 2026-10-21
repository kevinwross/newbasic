#include "newbasic/host.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>

#include "newbasic/host_path.h"

namespace newbasic {
namespace {

// localtime_s has the Microsoft (tm, time) argument order, while POSIX has
// localtime_r(time, tm). Keeping the branch here means the rest of the host
// code is identical on both platforms.
std::tm LocalTime(std::time_t now) {
  std::tm local{};
#if defined(_WIN32)
  if (localtime_s(&local, &now) != 0)
    local = {};
#else
  if (localtime_r(&now, &local) == nullptr)
    local = {};
#endif
  return local;
}

std::string GetEnv(const char* name) {
#if defined(_WIN32)
  char* value = nullptr;
  std::size_t length = 0;
  const bool found = _dupenv_s(&value, &length, name) == 0;
  const std::string result = (found && value != nullptr) ? std::string(value) : std::string();
  std::free(value);
  return result;
#else
  const char* value = std::getenv(name);
  return value != nullptr ? std::string(value) : std::string();
#endif
}

} // namespace

double SystemHost::SecondsSinceMidnight() {
  // TIMER is a single-precision number of seconds with a fractional part, the
  // way GW-BASIC's clock tick exposed it. Truncating to tm_sec made every demo
  // that paces itself on TIMER advance in one-second jumps.
  using Clock = std::chrono::system_clock;
  const Clock::time_point now = Clock::now();
  // Truncate first, then convert: to_time_t may round, which would make the
  // fractional part negative around a second boundary.
  const Clock::time_point whole_point =
      std::chrono::time_point_cast<std::chrono::seconds>(now);
  const std::tm local = LocalTime(Clock::to_time_t(whole_point));
  const double fraction = std::chrono::duration<double>(now - whole_point).count();
  return static_cast<double>(local.tm_hour) * 3600.0 +
         static_cast<double>(local.tm_min) * 60.0 + static_cast<double>(local.tm_sec) + fraction;
}

std::string SystemHost::DateString() {
  const std::tm local = LocalTime(std::time(nullptr));
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%02d-%02d-%04d", local.tm_mon + 1, local.tm_mday,
                local.tm_year + 1900);
  return buf;
}

std::string SystemHost::TimeString() {
  const std::tm local = LocalTime(std::time(nullptr));
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
  return buf;
}

void SystemHost::WriteToPrinter(const std::string& text) {
  // NEWBASIC_PRINTER names the sink; otherwise newbasic.prn in the working directory.
  // GW-BASIC sends this to LPT1; a hosted interpreter cannot assume a printer
  // exists, so a file is the honest equivalent.
  const std::string configured = GetEnv("NEWBASIC_PRINTER");
  const std::string path = !configured.empty() ? configured : std::string("newbasic.prn");
  std::ofstream out(HostPath(path), std::ios::app | std::ios::binary);
  out << text;
}

} // namespace newbasic
