// End-to-end checks of the shipped SCREEN 720 samples: each demo runs on a
// headless screen with a scripted ESC after one frame, so these are
// deterministic and need no window or GPU. The tests inspect the pixels and
// palette the BASIC code actually produced, and the interactions that change
// them (theme keys, new patterns, clicks) are exercised where they matter.
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "newbasic/frontend.h"
#include "newbasic/host.h"
#include "newbasic/line_store.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"

namespace {

std::string ReadAll(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.good());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

// Runs a sample with a scripted key sequence. The clock advances on every read,
// otherwise a demo that paces itself on TIMER would spin in its wait loop.
bool RunSample(const char* file, const std::vector<wchar_t>& keys,
               newbasic::screen::HeadlessScreen& screen, newbasic::RunResult* result) {
  const std::string source = ReadAll(std::string(NEWBASIC_SAMPLES_DIR) + "/" + file);
  newbasic::LineStore store;
  std::string parse_error;
  if (!newbasic::LineStore::ParseTextBas(source, &store, &parse_error))
    return false;

  newbasic::Program program;
  newbasic::CompileError error;
  if (!newbasic::Compile(store, &program, &error))
    return false;

  for (const wchar_t key : keys)
    screen.ScriptKey(key);
  newbasic::ScriptedHost host;
  host.timer_step = 0.05;
  newbasic::Vm vm(screen, &host);
  vm.Load(std::move(program));
  *result = vm.Run();
  return result->status == newbasic::RunResult::Status::Done;
}

int CountIndexRange(const newbasic::screen::Canvas& canvas, int low, int high) {
  int count = 0;
  for (int y = 0; y < canvas.Height(); ++y)
    for (int x = 0; x < canvas.Width(); ++x) {
      const int index = canvas.Pixel(x, y);
      if (index >= low && index <= high)
        ++count;
    }
  return count;
}

int DistinctColours(const newbasic::screen::Canvas& canvas) {
  bool seen[256] = {};
  for (int y = 0; y < canvas.Height(); ++y)
    for (int x = 0; x < canvas.Width(); ++x)
      seen[static_cast<std::size_t>(canvas.Pixel(x, y)) & 0xFF] = true;
  int count = 0;
  for (bool value : seen)
    count += value ? 1 : 0;
  return count;
}

std::uint64_t CanvasHash(const newbasic::screen::Canvas& canvas) {
  std::uint64_t hash = 1469598103934665603ull;
  for (int y = 0; y < canvas.Height(); ++y) {
    for (int x = 0; x < canvas.Width(); ++x) {
      hash ^= static_cast<std::uint64_t>(canvas.Pixel(x, y) & 0xFF);
      hash *= 1099511628211ull;
    }
  }
  return hash;
}

} // namespace

// ---------------------------------------------------------------- Neon Horizon

TEST_CASE("Sample: Neon Horizon paints a 1280x720 SCREEN 720 frame", "[sample][neon]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result{};
  REQUIRE(RunSample("NeonHorizon.bas", {0x1B}, screen, &result));

  CHECK(screen.GraphicsMode() == 720);
  const newbasic::screen::Canvas& canvas = screen.Graphics();
  REQUIRE(canvas.Width() == 1280);
  REQUIRE(canvas.Height() == 720);

  // Sky gradient 64..127, sun 128..159, ridges 176..178.
  const int sky = canvas.Pixel(100, 100);
  CHECK(sky >= 64);
  CHECK(sky <= 127);
  const int sun = canvas.Pixel(640, 350);
  INFO("sun pixel palette index: " << sun);
  CHECK(sun >= 128);
  CHECK(sun <= 159);
  const int ridge = canvas.Pixel(100, 490);
  CHECK(ridge >= 176);
  CHECK(ridge <= 178);
  const int floor = canvas.Pixel(5, 600);
  CHECK((floor == 16 || floor == 224 || floor == 225 || floor == 226));

  const auto& palette = screen.PaletteRgb();
  CHECK(palette[64] != palette[127]);
  CHECK(palette[128] != palette[159]);

  CHECK(screen.CellAt(1, 59) == L'N'); // title at LOCATE 2,60
  CHECK(screen.CellAt(43, 1) == L'E'); // controls at LOCATE 44,2
  CHECK_FALSE(screen.CursorVisible());
}

TEST_CASE("Sample: Neon Horizon cycles its palette on C", "[sample][neon]") {
  newbasic::screen::HeadlessScreen sunset;
  newbasic::RunResult result{};
  REQUIRE(RunSample("NeonHorizon.bas", {0x1B}, sunset, &result));

  newbasic::screen::HeadlessScreen aurora;
  REQUIRE(RunSample("NeonHorizon.bas", {L'c', 0x1B}, aurora, &result));

  CHECK(sunset.PaletteRgb()[64] != aurora.PaletteRgb()[64]);
  // The canvas stores the same indices; only the colours changed.
  CHECK(aurora.Graphics().Pixel(100, 100) == sunset.Graphics().Pixel(100, 100));
}

// -------------------------------------------------------------------- Ring World

TEST_CASE("Sample: Ring World paints a planet, rings and moons", "[sample][ringworld]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result{};
  REQUIRE(RunSample("RingWorld.bas", {0x1B}, screen, &result));

  CHECK(screen.GraphicsMode() == 720);
  const newbasic::screen::Canvas& canvas = screen.Graphics();
  REQUIRE(canvas.Width() == 1280);
  REQUIRE(canvas.Height() == 720);

  const int planet = canvas.Pixel(560, 360);
  INFO("planet pixel palette index: " << planet);
  CHECK(planet >= 64);
  CHECK(planet <= 159);

  // The ring plane crosses the far right of the planet; search a small box so
  // the test does not depend on one raster pixel of the ellipse.
  int ring_pixels = 0;
  for (int y = 340; y <= 380; ++y)
    for (int x = 900; x <= 980; ++x) {
      const int index = canvas.Pixel(x, y);
      if (index >= 160 && index <= 191)
        ++ring_pixels;
    }
  INFO("ring pixels near the right edge: " << ring_pixels);
  CHECK(ring_pixels > 10);

  const int sky = canvas.Pixel(100, 50);
  CHECK(sky >= 16);
  CHECK(sky <= 31);

  const int moon = canvas.Pixel(980, 190);
  CHECK((moon >= 192 && moon <= 194));

  CHECK(screen.CellAt(1, 65) == L'R'); // title at LOCATE 2,66
  CHECK(screen.CellAt(44, 1) == L'E'); // controls at LOCATE 45,2
  CHECK_FALSE(screen.CursorVisible());
}

TEST_CASE("Sample: Ring World cycles its palettes on C", "[sample][ringworld]") {
  newbasic::screen::HeadlessScreen gold;
  newbasic::RunResult result{};
  REQUIRE(RunSample("RingWorld.bas", {0x1B}, gold, &result));

  newbasic::screen::HeadlessScreen ice;
  REQUIRE(RunSample("RingWorld.bas", {L'c', 0x1B}, ice, &result));

  CHECK(gold.PaletteRgb()[64] != ice.PaletteRgb()[64]);
  const int gold_planet = gold.Graphics().Pixel(560, 360);
  const int ice_planet = ice.Graphics().Pixel(560, 360);
  CHECK(gold_planet == ice_planet); // indices stable, palette changed
}

// -------------------------------------------------------------------- Fireworks

TEST_CASE("Sample: Fireworks paints seeded bursts and a city", "[sample][fireworks]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result{};
  REQUIRE(RunSample("Fireworks.bas", {0x1B}, screen, &result));

  CHECK(screen.GraphicsMode() == 720);
  const newbasic::screen::Canvas& canvas = screen.Graphics();
  REQUIRE(canvas.Width() == 1280);
  REQUIRE(canvas.Height() == 720);

  // Count the five firework families (8 shades each) and the other layers.
  int family[5] = {};
  int sky = 0;
  int stars = 0;
  int city = 0;
  for (int y = 0; y < canvas.Height(); ++y) {
    for (int x = 0; x < canvas.Width(); ++x) {
      const int index = canvas.Pixel(x, y);
      if (index >= 64 && index <= 103)
        ++family[(index - 64) / 8];
      else if (index >= 16 && index <= 31)
        ++sky;
      else if (index >= 32 && index <= 35)
        ++stars;
      else if (index >= 192 && index <= 194)
        ++city;
    }
  }
  int families = 0;
  for (int count : family) {
    INFO("family count: " << count);
    if (count > 100)
      ++families;
  }
  INFO("firework pixels: " << (family[0] + family[1] + family[2] + family[3] + family[4]));
  CHECK(families >= 4);
  CHECK((family[0] + family[1] + family[2] + family[3] + family[4]) > 1000);
  CHECK(sky > 10000);
  CHECK(stars > 20);
  CHECK(city > 1000);

  CHECK(screen.CellAt(1, 61) == L'F'); // title at LOCATE 2,62
  CHECK(screen.CellAt(44, 1) == L'E'); // controls at LOCATE 45,2
  CHECK_FALSE(screen.CursorVisible());
}

// ------------------------------------------------------------------ Spirograph

TEST_CASE("Sample: Spirograph draws a coloured rosette", "[sample][spirograph]") {
  newbasic::screen::HeadlessScreen screen;
  newbasic::RunResult result{};
  REQUIRE(RunSample("Spirograph.bas", {0x1B}, screen, &result));

  CHECK(screen.GraphicsMode() == 720);
  const newbasic::screen::Canvas& canvas = screen.Graphics();
  REQUIRE(canvas.Width() == 1280);
  REQUIRE(canvas.Height() == 720);

  // The hue wheel occupies 16..255; a rosette uses many of those entries.
  CHECK(CountIndexRange(canvas, 16, 255) > 5000);
  CHECK(DistinctColours(canvas) > 50);
  CHECK(canvas.Pixel(640, 360) == 0); // the middle of the rosette is open

  CHECK(screen.CellAt(1, 57) == L'S'); // title at LOCATE 2,58
  CHECK(screen.CellAt(44, 1) == L'E'); // controls at LOCATE 45,2
  CHECK_FALSE(screen.CursorVisible());
}

TEST_CASE("Sample: Spirograph draws a new pattern on SPACE", "[sample][spirograph]") {
  newbasic::screen::HeadlessScreen first;
  newbasic::RunResult result{};
  REQUIRE(RunSample("Spirograph.bas", {0x1B}, first, &result));

  newbasic::screen::HeadlessScreen second;
  REQUIRE(RunSample("Spirograph.bas", {L' ', 0x1B}, second, &result));

  CHECK(CanvasHash(first.Graphics()) != CanvasHash(second.Graphics()));
  CHECK(CountIndexRange(second.Graphics(), 16, 255) > 5000);
}
