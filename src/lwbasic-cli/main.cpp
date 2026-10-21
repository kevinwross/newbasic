// lwbasic.exe — GW-BASIC dialect interpreter.
//
//   lwbasic.exe                 direct-mode session (REPL)
//   lwbasic.exe prog.bas        load a plain-text .BAS file and RUN it
//   lwbasic.exe -? | --help     usage

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "lwbasic/errors.h"
#include "lwbasic/frontend.h"
#include "lwbasic/host.h"
#include "lwbasic/line_store.h"
#include "lwbasic/repl.h"
#include "lwbasic/screen/console_screen.h"
#include "lwbasic/screen/d3d11_screen.h"
#include "lwbasic/screen/glyph_atlas.h"
#include "lwbasic/screen/headless_screen.h"
#include "lwbasic/screen/text_render.h"
#include "lwbasic/vm.h"

namespace {

// The windowed D3D11 backend is opt-in (--gpu) and best-effort: when it cannot be
// created the console backend is used instead, with one line saying why. See
// docs/m7-d3d11-backend.md.
bool g_use_gpu = true; // the window is the default front end; --cli asks for the console
// Window scale, as a multiple of the 8x16 text cell (--gpu-scale).
int g_gpu_scale = 1;
// When set, the canvas is written to this file after the run (--gpu-shot).
std::string g_shot_path;
// Keys to feed the windowed terminal before it runs (--keys), so a session can be scripted.
std::string g_keys;
// Scripted clicks for a session (--clicks): "x,y,button" entries separated by semicolons.
std::string g_clicks;
// One canvas pixel to read back from the window after the run (--gpu-pixel x,y). This is the
// only check that looks at what the window actually presented: the view dump is written by
// separate code, so it can disagree with the display, and twice it did.
std::string g_pixel_probe;

// Reads the requested pixel from the window's back buffer and reports it. Doing nothing when
// --gpu-pixel was not given keeps the call sites free of conditionals.
void ProbeWindowPixel(lwbasic::screen::Screen& screen, bool is_console) {
  if (g_pixel_probe.empty())
    return;
  int x = 0;
  int y = 0;
  if (sscanf_s(g_pixel_probe.c_str(), "%d,%d", &x, &y) < 2) {
    std::fprintf(stderr, "lwbasic: --gpu-pixel takes x,y\n");
    return;
  }
  if (is_console) {
    std::fprintf(stderr, "lwbasic: --gpu-pixel needs the window backend\n");
    return;
  }
  auto* window = static_cast<lwbasic::screen::D3D11Screen*>(&screen);
  std::uint8_t rgba[4] = {0, 0, 0, 0};
  if (!window->ReadBackCanvasPixel(x, y, rgba)) {
    std::fprintf(stderr, "lwbasic: --gpu-pixel could not read %d,%d\n", x, y);
    return;
  }
  std::fprintf(stderr, "lwbasic: window pixel %d,%d = %d,%d,%d\n", x, y, rgba[0], rgba[1], rgba[2]);
}

// Writes what the screen shows as a binary PPM: the graphics canvas with the text layer
// drawn over it, or just the text for a text-mode program. This is how a screen can be
// checked without a screenshot, which matters because a windowed capture is not possible
// everywhere. The text is rasterised from the same glyph atlas the window uses.
bool WriteViewPpm(const lwbasic::screen::Screen& screen, const std::string& path) {
  const lwbasic::screen::Canvas& canvas = screen.Graphics();
  // Match the device cell size, otherwise a screenshot is always 8x16 no matter how the
  // window was scaled.
  const lwbasic::screen::GlyphAtlas atlas =
      lwbasic::screen::RenderGlyphAtlas(screen.GlyphWidth(), screen.GlyphHeight());
  // The canvas outlives a mode change, so the mode decides, not the canvas size.
  const bool graphics = screen.GraphicsMode() != lwbasic::screen::Screen::kTextMode &&
                        canvas.Width() > 0 && canvas.Height() > 0;
  const int width = graphics ? canvas.Width() : screen.Width() * atlas.glyph_width;
  const int height = graphics ? canvas.Height() : screen.Height() * atlas.glyph_height;
  if (width <= 0 || height <= 0)
    return false;

  const std::array<std::array<std::uint8_t, 3>, 256>& palette = screen.PaletteRgb();
  std::vector<std::uint8_t> image(
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3, 0);
  auto set = [&](int x, int y, const std::array<std::uint8_t, 3>& colour) {
    if (x < 0 || y < 0 || x >= width || y >= height)
      return;
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x)) *
                               3;
    image[offset + 0] = colour[0];
    image[offset + 1] = colour[1];
    image[offset + 2] = colour[2];
  };

  if (graphics) {
    for (int y = 0; y < height; ++y)
      for (int x = 0; x < width; ++x)
        set(x, y, palette[static_cast<std::size_t>(canvas.Pixel(x, y))]);
  }

  // Text. In a graphics mode the glyphs sit over the drawing with no background, which is
  // what the window does; in text mode the cell background is filled first.
  for (int row = 0; row < screen.Height(); ++row) {
    for (int col = 0; col < screen.Width(); ++col) {
      const wchar_t ch = screen.CellAt(row, col);
      const std::uint8_t attr =
          screen.AttrGrid()[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
      const bool wide_mode = screen.IsTrueColourMode();
      // The window draws the cursor cell in inverted video, so the screenshot does the same:
      // a block cursor is what a text-mode terminal shows, and without it the dump would
      // look like the caret was missing.
      const bool cursor_cell =
          screen.CursorVisible() && (row == screen.CursorRow() && col == screen.CursorCol());
      std::array<std::uint8_t, 3> foreground =
          palette[static_cast<std::size_t>(wide_mode ? attr : (attr & 0x0F))];
      std::array<std::uint8_t, 3> background =
          palette[static_cast<std::size_t>(wide_mode ? attr : ((attr >> 4) & 0x0F))];
      if (cursor_cell) {
        const std::array<std::uint8_t, 3> swap = foreground;
        foreground = background;
        background = swap;
      }
      const int cell_x = col * atlas.glyph_width;
      const int cell_y = row * atlas.glyph_height;
      if (!graphics || cursor_cell) {
        // The cursor cell needs its block painted even over graphics, which is what the
        // window does through the inverted background quad.
        for (int py = 0; py < atlas.glyph_height; ++py)
          for (int px = 0; px < atlas.glyph_width; ++px)
            set(cell_x + px, cell_y + py, background);
      }
      if (ch == L' ' || !atlas.ok)
        continue;
      const int glyph = static_cast<int>(lwbasic::screen::UnicodeToCp437(ch));
      const int glyph_x = (glyph % 16) * atlas.glyph_width;
      const int glyph_y = (glyph / 16) * atlas.glyph_height;
      for (int py = 0; py < atlas.glyph_height; ++py) {
        for (int px = 0; px < atlas.glyph_width; ++px) {
          // Blend by coverage rather than thresholding it, so the text keeps the atlas
          // antialiasing and looks like the window instead of a ragged cut-out.
          const int coverage = atlas.At(glyph_x + px, glyph_y + py);
          const int x = cell_x + px;
          const int y = cell_y + py;
          if (coverage == 0 || x < 0 || y < 0 || x >= width || y >= height)
            continue;
          const std::size_t offset =
              (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
               static_cast<std::size_t>(x)) *
              3;
          for (int channel = 0; channel < 3; ++channel) {
            const int under = image[offset + static_cast<std::size_t>(channel)];
            image[offset + static_cast<std::size_t>(channel)] = static_cast<std::uint8_t>(
                (foreground[static_cast<std::size_t>(channel)] * coverage +
                 under * (255 - coverage)) /
                255);
          }
        }
      }
    }
  }

  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;
  out << "P6\n" << width << " " << height << "\n255\n";
  out.write(reinterpret_cast<const char*>(image.data()),
            static_cast<std::streamsize>(image.size()));
  return static_cast<bool>(out);
}

// Feeds scripted clicks to the window, in canvas pixels.
void ApplyScriptedClicks(lwbasic::screen::D3D11Screen* screen) {
  if (screen == nullptr || g_clicks.empty())
    return;
  std::size_t start = 0;
  while (start < g_clicks.size()) {
    const std::size_t end = g_clicks.find(';', start);
    const std::string entry =
        g_clicks.substr(start, end == std::string::npos ? std::string::npos : end - start);
    int x = 0;
    int y = 0;
    int button = 1;
    if (sscanf_s(entry.c_str(), "%d,%d,%d", &x, &y, &button) >= 2)
      screen->InjectClick(x, y, button);
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
}

struct ScreenChoice {
  std::unique_ptr<lwbasic::screen::Screen> gpu;
  lwbasic::screen::ConsoleScreen console;
  lwbasic::screen::Screen* screen = nullptr;
  bool is_console = false;
};

bool ChooseScreen(std::atomic<bool>* break_flag, ScreenChoice* choice) {
  if (g_use_gpu) {
    std::string error;
    choice->gpu = lwbasic::screen::D3D11Screen::Create(&error, g_gpu_scale);
    if (choice->gpu != nullptr) {
      // A scripted session feeds the same queue the window fills, so it exercises the real
      // input path rather than a shortcut.
      if (!g_keys.empty()) {
        std::wstring wide;
        for (const char c : g_keys)
          wide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
        static_cast<lwbasic::screen::D3D11Screen*>(choice->gpu.get())->InjectKeys(wide);
      }
      // Scripted clicks go through the same queue a real click fills, so the program
      // cannot tell the difference.
      ApplyScriptedClicks(static_cast<lwbasic::screen::D3D11Screen*>(choice->gpu.get()));
      choice->screen = choice->gpu.get();
      return true;
    }
    std::fprintf(stderr, "lwbasic: --gpu unavailable (%s); using the console.\n", error.c_str());
  }
  if (!choice->console.Init(break_flag)) {
    std::fprintf(stderr, "lwbasic requires a console (conhost or Windows Terminal).\n");
    return false;
  }
  choice->screen = &choice->console;
  choice->is_console = true;
  return true;
}

const char* kUsage =
    "lwbasic [options] [program.bas]\n"
    "\n"
    "A terminal BASIC interpreter for the GW-BASIC dialect.\n"
    "\n"
    "Options:\n"
    "  -? | --help      Show this help.\n"
    "  --gpu            Render in a D3D11 window. This is the default.\n"
    "  --cli            Use the console instead of the window (the old default).\n"
    "  --gpu-scale N    Window size as a multiple of the 8x16 text cell:\n"
    "                   1 is 640x400 at 96 DPI, 2 is 1280x800.\n"
    "  --keys KEYS      Feed KEYS to the windowed terminal as typed input, so a session\n"
    "                   can be scripted; a CR in KEYS is Enter.\n"
    "  --gpu-shot FILE  After the run, write what the screen shows to FILE as a binary\n"
    "                   PPM, so a plot can be checked without a screenshot.\n"
    "  --gpu-pixel X,Y  After the run, read one pixel back from the window itself and print\n"
    "                   it. The dump is written by separate code, so it can differ from the\n"
    "                   display; this reads what was actually presented.\n"
    "  --audio-check    Play a short arpeggio and exit, so the BEEP path can be\n"
    "                   checked by ear without a program file.\n"
    "  --gpu-check      Render one known frame in the window, read the pixels back\n"
    "                   and report PASS or FAIL. Use it to tell a rendering fault\n"
    "                   from a window that is not receiving the swap chain.\n"
    "\n"
    "With a file, lwbasic loads the program and runs it. Without one it\n"
    "starts a direct-mode session (RUN, LIST, NEW, CLEAR, SYSTEM, or any\n"
    "statement).\n";

// Reports a finished run the way GW-BASIC does.
void ReportResult(const lwbasic::RunResult& result) {
  switch (result.status) {
  case lwbasic::RunResult::Status::Done:
    break;
  case lwbasic::RunResult::Status::Break:
    std::printf("Break in %d\n", result.line);
    break;
  case lwbasic::RunResult::Status::Error:
    std::printf("%s\n", lwbasic::FormatError(result.code, result.line).c_str());
    break;
  }
}

// Loads a plain-text .BAS file, compiles it and runs it.
int RunFile(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "Can't open file: %s\n", path);
    return 1;
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();

  lwbasic::LineStore store;
  std::string error;
  if (!lwbasic::LineStore::ParseTextBas(text, &store, &error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
  lwbasic::Program program;
  lwbasic::CompileError cerr;
  if (!lwbasic::Compile(store, &program, &cerr)) {
    std::fprintf(stderr, "%s\n", lwbasic::FormatError(cerr.code, cerr.line).c_str());
    return 1;
  }

  std::atomic<bool> break_flag{false};
  ScreenChoice choice;
  if (!ChooseScreen(&break_flag, &choice))
    return 1;
  lwbasic::screen::Screen& screen = *choice.screen;
  lwbasic::SystemHost host;
  screen.EnterFullScreen();
  lwbasic::Vm vm(screen, &host, &break_flag);
  // The program-editing statements (LIST/DELETE/NEW/SAVE/LOAD/CHAIN) act on the
  // stored source lines, so the VM needs the store the program came from.
  vm.SetLineStore(&store);
  vm.Load(std::move(program));
  const lwbasic::RunResult result = vm.Run();
  // The final frame must show even if the throttle skipped it moments ago.
  screen.PresentNow();
  ProbeWindowPixel(screen, choice.is_console);
  if (!g_shot_path.empty()) {
    // Written after the run, which ends when the window is closed, so the file holds
    // the finished picture.
    if (WriteViewPpm(screen, g_shot_path)) {
      std::fprintf(stderr, "lwbasic: view written to %s\n", g_shot_path.c_str());
    } else {
      std::fprintf(stderr, "lwbasic: could not write %s\n", g_shot_path.c_str());
    }
  }
  screen.ExitFullScreen();
  ReportResult(result);
  return result.status == lwbasic::RunResult::Status::Done ? 0 : 1;
}

// Direct-mode session.
int RunRepl() {
  std::atomic<bool> break_flag{false};
  ScreenChoice choice;
  if (!ChooseScreen(&break_flag, &choice))
    return 1;
  lwbasic::screen::Screen& screen = *choice.screen;
  lwbasic::SystemHost host;
  screen.EnterFullScreen();
  if (choice.is_console)
    choice.console.ShowCaret(true); // direct mode shows a cursor
  lwbasic::Repl repl(screen, &host);
  while (repl.Step()) {
  }
  if (choice.is_console)
    choice.console.ShowCaret(false);
  ProbeWindowPixel(screen, choice.is_console);
  // A session ends with SYSTEM or a closed window; either way a requested screenshot is
  // written here as well as after a program run.
  if (!g_shot_path.empty()) {
    if (WriteViewPpm(screen, g_shot_path))
      std::fprintf(stderr, "lwbasic: view written to %s\n", g_shot_path.c_str());
    else
      std::fprintf(stderr, "lwbasic: could not write %s\n", g_shot_path.c_str());
  }
  screen.ExitFullScreen();
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  int first = 1;
  while (first < argc) {
    if (std::strcmp(argv[first], "--gpu") == 0) {
      g_use_gpu = true;
      ++first;
      continue;
    }
    if (std::strcmp(argv[first], "--cli") == 0) {
      // The console backend: what this program used to do by default.
      g_use_gpu = false;
      ++first;
      continue;
    }
    if (first + 1 < argc && std::strcmp(argv[first], "--gpu-scale") == 0) {
      // Window size as a multiple of the 8x16 text cell: 1 is 640x400 at 96 DPI.
      g_gpu_scale = std::atoi(argv[first + 1]);
      if (g_gpu_scale < 1 || g_gpu_scale > 8) {
        std::fprintf(stderr, "lwbasic: --gpu-scale takes a value from 1 to 8\n");
        return 1;
      }
      g_use_gpu = true;
      first += 2;
      continue;
    }
    if (first + 1 < argc && std::strcmp(argv[first], "--keys") == 0) {
      g_keys = argv[first + 1];
      g_use_gpu = true;
      first += 2;
    }
    if (first + 1 < argc && std::strcmp(argv[first], "--gpu-shot") == 0) {
      g_shot_path = argv[first + 1];
      g_use_gpu = true;
      first += 2;
      continue;
    }
    if (first + 1 < argc && std::strcmp(argv[first], "--clicks") == 0) {
      g_clicks = argv[first + 1];
      first += 2;
      continue;
    }
    if (first + 1 < argc && std::strcmp(argv[first], "--gpu-pixel") == 0) {
      g_pixel_probe = argv[first + 1];
      first += 2;
      continue;
    }
    break;
  }
  if (first < argc && std::strcmp(argv[first], "--bench") == 0) {
    // Measures the interpreter on fixed workloads. Performance work on this project was
    // being reasoned about instead of measured, which is how a plotting program took 21
    // seconds while the drawing code looked fine.
    lwbasic::screen::HeadlessScreen screen;
    lwbasic::SystemHost host;
    struct Case {
      const char* name;
      const char* program;
      int operations; // the count the program performs, for operations per second
    };
    const Case cases[] = {
        {"integer loop: FOR I = 1 TO 200000: X = X + 1: NEXT",
         "10 X = 0\n20 FOR I = 1 TO 200000\n30 X = X + 1\n40 NEXT I\n50 END\n", 200000},
        {"float loop: 20000 x (4 multiplies, 3 adds)",
         "10 X = 1.5: Y = 0.25\n20 FOR I = 1 TO 20000\n30 X = X * 0.5 + Y * 0.25\n"
         "40 Y = Y * 0.5 + X * 0.25\n50 NEXT I\n60 END\n",
         20000},
        {"mandelbrot core: 2000 cells x 24 iterations",
         "10 MX = 24\n20 FOR PY = 0 TO 49\n30 CY = -1 + PY * 0.04\n40 FOR PX = 0 TO 39\n"
         "50 CX = -2 + PX * 0.06\n60 ZX = 0: ZY = 0: IT = 0\n"
         "70 WHILE IT < MX AND ZX * ZX + ZY * ZY <= 4\n"
         "80 T = ZX * ZX - ZY * ZY + CX\n90 ZY = 2 * ZX * ZY + CY\n100 ZX = T\n"
         "110 IT = IT + 1\n120 WEND\n130 NEXT PX\n140 NEXT PY\n150 END\n",
         2000},
        {"PSET: 20000 pixels on a 320x200 canvas",
         "10 SCREEN 1\n20 FOR I = 1 TO 20000\n30 PSET (I MOD 320, I MOD 200), 1\n"
         "40 NEXT I\n50 END\n",
         20000},
    };
    std::printf("lwbasic benchmark (headless, so no window cost)\n");
    for (const Case& bench : cases) {
      lwbasic::Repl repl(screen, &host);
      const auto start = std::chrono::steady_clock::now();
      // Feed the program as numbered lines, then run it.
      std::string text(bench.program);
      std::size_t begin = 0;
      while (begin < text.size()) {
        const std::size_t end = text.find('\n', begin);
        const std::string line =
            text.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
        if (!line.empty())
          repl.Process(line);
        if (end == std::string::npos)
          break;
        begin = end + 1;
      }
      const lwbasic::RunResult result = repl.Process("RUN");
      const auto finish = std::chrono::steady_clock::now();
      const double seconds = std::chrono::duration<double>(finish - start).count();
      const double per_second = seconds > 0.0 ? bench.operations / seconds : 0.0;
      if (result.status == lwbasic::RunResult::Status::Done) {
        std::printf("  %-52s %7.3f s  %10.0f ops/s\n", bench.name, seconds, per_second);
      } else {
        // A benchmark that silently measures a failed run is worse than none.
        std::printf("  %-52s FAILED: status %d, line %d\n", bench.name,
                    static_cast<int>(result.status), result.line);
      }
    }
    return 0;
  }
  if (first < argc && std::strcmp(argv[first], "--audio-check") == 0) {
    // A scriptable check of the Beep path: no program file and no screen needed, so
    // the whole BEEP -> Host::PlayTone -> Beep() chain can be exercised by ear.
    lwbasic::SystemHost host;
    std::printf("playing A4 (440 Hz), C#5 (554 Hz), E5 (659 Hz)\n");
    host.PlayTone(440, 400);
    host.PlayTone(554, 400);
    host.PlayTone(659, 700);
    std::printf("done\n");
    return 0;
  }
  if (first < argc && std::strcmp(argv[first], "--gpu-check") == 0) {
    // Renders one known frame through the windowed backend, reads the pixels back and
    // reports. This is the check to run on any machine: it separates "the GPU output is
    // right" from "the window shows it", which is the pair that confused this project
    // once already.
    std::string error;
    const std::unique_ptr<lwbasic::screen::Screen> screen =
        lwbasic::screen::D3D11Screen::Create(&error, g_gpu_scale);
    if (screen == nullptr) {
      std::printf("gpu-check: no windowed backend (%s)\n", error.c_str());
      return 1;
    }
    auto* d3d = static_cast<lwbasic::screen::D3D11Screen*>(screen.get());
    screen->SetGraphicsMode(1);
    screen->ClearGraphics(0);
    screen->Plot(160, 100, 4); // red at the canvas centre
    screen->Plot(10, 10, 2);   // colour 2 is red in SCREEN 1's palette too
    screen->Present();
    unsigned char centre[4] = {0, 0, 0, 0};
    unsigned char corner[4] = {255, 255, 255, 255};
    const bool centre_ok = d3d->ReadBackCanvasPixel(160, 100, centre);
    const bool corner_ok = d3d->ReadBackCanvasPixel(200, 150, corner);
    const bool centre_red = centre_ok && centre[0] > 120 && centre[1] < 90 && centre[2] < 90;
    const bool corner_black = corner_ok && corner[0] < 60 && corner[1] < 60 && corner[2] < 60;
    std::printf("gpu-check: centre=%d,%d,%d corner=%d,%d,%d\n", centre[0], centre[1], centre[2],
                corner[0], corner[1], corner[2]);
    std::printf("gpu-check: %s\n", (centre_red && corner_black) ? "PASS" : "FAIL");
    std::printf("gpu-check: the window should now show a red dot on black; if the read back "
                "passes but the window is blank, the swap chain is not reaching the display\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return (centre_red && corner_black) ? 0 : 1;
  }
  const int remaining = argc - first;
  if (remaining >= 1 &&
      (std::strcmp(argv[first], "-?") == 0 || std::strcmp(argv[first], "--help") == 0)) {
    std::fputs(kUsage, stdout);
    return 0;
  }
  if (remaining > 1) {
    std::fputs(kUsage, stdout);
    return 1;
  }
  if (remaining == 1)
    return RunFile(argv[first]);
  return RunRepl();
}
