#include "newbasic/host.h"

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#include "newbasic/sound.h"

namespace newbasic {

bool SystemHost::CanRunInteractiveShell() const {
  return GetConsoleWindow() != nullptr;
}

int SystemHost::RunShell(const std::string& command) {
  // An empty SHELL needs a console to attach to. Without one, cmd.exe would
  // wait for input that can never arrive.
  if (command.empty() && !CanRunInteractiveShell())
    return -1;
  std::wstring wide;
  if (!command.empty()) {
    const int needed = MultiByteToWideChar(CP_ACP, 0, command.c_str(),
                                           static_cast<int>(command.size()), nullptr, 0);
    if (needed > 0) {
      wide.resize(static_cast<std::size_t>(needed));
      MultiByteToWideChar(CP_ACP, 0, command.c_str(), static_cast<int>(command.size()), wide.data(),
                          needed);
    }
  }
  std::wstring command_line = command.empty() ? L"cmd.exe" : (L"cmd.exe /c " + wide);
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION process{};
  if (CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
                     &startup, &process) == 0) {
    return -1;
  }
  WaitForSingleObject(process.hProcess, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(process.hProcess, &exit_code);
  CloseHandle(process.hProcess);
  CloseHandle(process.hThread);
  return static_cast<int>(exit_code);
}

void SystemHost::PlayTone(int frequency_hz, int duration_ms) {
  // BEEP/SOUND/PLAY are synchronous in the dialect (foreground is the default), so this
  // blocks for the tone's length. Silence is still a wait, and the duration is capped so a
  // bad PLAY string cannot hang the interpreter for minutes.
  //
  // The tone itself is synthesised (see RenderTone) and played through waveOut rather than
  // Beep. Beep is the PC-speaker emulation: a square wave with no envelope, which is why a
  // melody played through it sounds like a dial tone. One device is opened on first use and
  // kept, because opening a device per note costs more than the note's own start-up.
  const int capped_ms = duration_ms < 0 ? 0 : (duration_ms > 5000 ? 5000 : duration_ms);
  if (frequency_hz <= 0) {
    if (capped_ms > 0)
      Sleep(static_cast<DWORD>(capped_ms));
    return;
  }
  const int frequency = frequency_hz < 37 ? 37 : (frequency_hz > 32767 ? 32767 : frequency_hz);
  // A zero duration means "until the next sound" in the dialect; a short fixed tone is the
  // closest bounded approximation (documented divergence).
  const int milliseconds = capped_ms <= 0 ? 100 : capped_ms;
  const std::vector<std::int16_t> samples = RenderTone(frequency, milliseconds);

  static std::mutex mutex;
  const std::lock_guard<std::mutex> lock(mutex);
  static HWAVEOUT device = nullptr;
  static WAVEFORMATEX format = {};
  if (device == nullptr) {
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 44100;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;
    if (waveOutOpen(&device, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
      device = nullptr;
      Beep(static_cast<DWORD>(frequency), static_cast<DWORD>(milliseconds)); // no device: fall back
      return;
    }
  }
  WAVEHDR header = {};
  header.lpData = reinterpret_cast<LPSTR>(const_cast<std::int16_t*>(samples.data()));
  header.dwBufferLength = static_cast<DWORD>(samples.size() * sizeof(std::int16_t));
  if (waveOutPrepareHeader(device, &header, sizeof(header)) != MMSYSERR_NOERROR) {
    Beep(static_cast<DWORD>(frequency), static_cast<DWORD>(milliseconds));
    return;
  }
  if (waveOutWrite(device, &header, sizeof(header)) != MMSYSERR_NOERROR) {
    waveOutUnprepareHeader(device, &header, sizeof(header));
    Beep(static_cast<DWORD>(frequency), static_cast<DWORD>(milliseconds));
    return;
  }
  // Block until the buffer has played, which is what foreground PLAY means. Polling with a 1 ms
  // sleep is accurate enough for musical timing and cheaper than a callback.
  while ((header.dwFlags & WHDR_DONE) == 0)
    Sleep(1);
  waveOutUnprepareHeader(device, &header, sizeof(header));
}

} // namespace newbasic
