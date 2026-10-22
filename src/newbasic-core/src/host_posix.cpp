#include "newbasic/host.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace newbasic {

bool SystemHost::CanRunInteractiveShell() const {
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

int SystemHost::RunShell(const std::string& command) {
  // An empty SHELL starts an interactive session. std::system always adds
  // "sh -c", which is wrong for an interactive shell, so exec the user's
  // shell directly. Without a terminal there is nothing to attach to; the VM
  // normally rejects this earlier, but guard the host as well.
  if (command.empty()) {
    if (!CanRunInteractiveShell())
      return -1;
    const char* configured = std::getenv("SHELL");
    const std::string shell =
        (configured != nullptr && *configured != '\0') ? std::string(configured) : "/bin/sh";
    const pid_t pid = fork();
    if (pid < 0)
      return -1;
    if (pid == 0) {
      execl(shell.c_str(), shell.c_str(), static_cast<char*>(nullptr));
      _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
      if (errno != EINTR)
        return -1;
    }
    if (WIFEXITED(status))
      return WEXITSTATUS(status);
    return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
  }

  const int status = std::system(command.c_str());
  if (status == -1)
    return -1;
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return -1;
}

void SystemHost::PlayTone(int frequency_hz, int duration_ms) {
  // The dialect's sound statements are synchronous, so the interpreter still
  // has to wait for the requested duration even when no audio device is used.
  // This keeps PLAY/SOUND/BEEP timing identical on a headless machine.
  //
  // An optional ALSA output can be added here later; the default build stays
  // dependency-free and silent.
  const int capped_ms = duration_ms < 0 ? 0 : (duration_ms > 5000 ? 5000 : duration_ms);
  const int milliseconds = (frequency_hz > 0 && capped_ms <= 0) ? 100 : capped_ms;
  if (milliseconds > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }
}

} // namespace newbasic
