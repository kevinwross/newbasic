#pragma once

#include <string>
#include <utility>
#include <vector>

namespace newbasic {

// Host services the VM needs from its environment (plan key seams): clock and
// date/time now; filesystem and child processes in M3. Tests use ScriptedHost
// so runs stay deterministic.
class Host {
public:
  virtual ~Host() = default;
  // Seconds since midnight (TIMER function).
  virtual double SecondsSinceMidnight() { return 0.0; }
  // Local date as MM-DD-YYYY (DATE$).
  virtual std::string DateString() { return "01-01-1980"; }
  // Local time as HH:MM:SS (TIME$).
  virtual std::string TimeString() { return "00:00:00"; }
  // Environment snapshot for ENVIRON$. Names are returned as the host spells
  // them; the VM normalises them to uppercase for lookup.
  virtual std::vector<std::pair<std::string, std::string>> EnvironmentSnapshot() {
    return {};
  }
  // Runs a SHELL command line through the operating system's command
  // interpreter (an empty command starts an interactive session). Returns
  // the exit code, or -1 when the interpreter could not be started.
  virtual int RunShell(const std::string& command) {
    (void)command;
    return -1;
  }
  // True when an empty SHELL (an interactive session) can attach to a
  // terminal. Windowed/headless runs must return false here: spawning an
  // interactive shell without a terminal leaves newbasic waiting forever.
  virtual bool CanRunInteractiveShell() const { return true; }
  // Plays a tone (M7). frequency_hz 0 means silence for the duration. The base
  // host ignores this, so headless runs and tests stay silent.
  virtual void PlayTone(int frequency_hz, int duration_ms) {
    (void)frequency_hz;
    (void)duration_ms;
  }
  // Printer output (LLIST and LPRINT). The base host discards it, so headless runs
  // and tests stay silent; SystemHost appends to a file and ScriptedHost records it.
  virtual void WriteToPrinter(const std::string& text) { (void)text; }
};

// Real clock-backed host used by newbasic.exe.
class SystemHost : public Host {
public:
  double SecondsSinceMidnight() override;
  std::string DateString() override;
  std::string TimeString() override;
  std::vector<std::pair<std::string, std::string>> EnvironmentSnapshot() override;
  int RunShell(const std::string& command) override;
  bool CanRunInteractiveShell() const override;
  void PlayTone(int frequency_hz, int duration_ms) override;
  // Appends to the file named by NEWBASIC_PRINTER, or newbasic.prn in the working
  // directory. GW-BASIC sends this to LPT1; a hosted interpreter cannot assume a
  // printer exists, so a file is the honest equivalent.
  void WriteToPrinter(const std::string& text) override;
};

// Deterministic host for tests.
class ScriptedHost : public Host {
public:
  double timer = 0.0;
  std::string date = "01-01-1980";
  std::string time = "00:00:00";
  std::vector<std::pair<std::string, std::string>> environment;

  std::string DateString() override { return date; }
  std::string TimeString() override { return time; }
  double SecondsSinceMidnight() override { return ReadClock(); }

  // Records SHELL commands instead of running them (tests).
  std::vector<std::string> shell_commands;
  int shell_result = 0;
  int RunShell(const std::string& command) override {
    shell_commands.push_back(command);
    return shell_result;
  }

  // Records tones instead of sounding them (tests). Playback is synchronous, so
  // the recorded order is the order the tones would have been played.
  std::vector<std::pair<int, int>> tones; // (frequency_hz, duration_ms)
  void PlayTone(int frequency_hz, int duration_ms) override {
    tones.emplace_back(frequency_hz, duration_ms);
  }

  // Records printer output instead of writing it (tests).
  std::string printer;
  void WriteToPrinter(const std::string& text) override { printer += text; }

  // Scripted clock: every read advances `timer` by `timer_step`, so timer
  // event tests are deterministic.
  double timer_step = 0.0;
  std::vector<std::pair<std::string, std::string>> EnvironmentSnapshot() override {
    return environment;
  }
  double ReadClock() {
    const double value = timer;
    timer += timer_step;
    return value;
  }
};

} // namespace newbasic
