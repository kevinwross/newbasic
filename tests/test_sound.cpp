// M7: sound - BEEP, SOUND and PLAY through the host seam.
//
// All audio goes through Host::PlayTone, so these tests use ScriptedHost and
// record tones instead of making noise: the suite stays silent and CI-safe.
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "lwbasic/errors.h"
#include "lwbasic/host.h"
#include "lwbasic/sound.h"
#include "lwbasic/repl.h"
#include "lwbasic/screen/headless_screen.h"
#include "test_helpers.h"

// ---------------------------------------------------------------------------
// Manual audio checks. The leading dot hides these from CI (Catch2 skips
// [.tag] tests unless they are named explicitly) because they make real sound
// and would beep on a build machine. Run them by hand:
//
//   out\tests\Release\lwbasic-tests.exe "[.audio]" -s
//
// The other sound tests use ScriptedHost and are silent by design, which is
// exactly why the audio path itself needs this separate, explicit check.
// ---------------------------------------------------------------------------
TEST_CASE("Sound: the real host blocks for the tone duration", "[.audio]") {
  lwbasic::SystemHost host;
  const auto start = std::chrono::steady_clock::now();
  host.PlayTone(800, 250); // the BEEP tone
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
  WARN("SystemHost::PlayTone(800, 250) took " << elapsed << " ms");
  // Beep() is synchronous, so blocking for at least ~150 ms proves the call
  // reached the audio device instead of being a silent no-op.
  CHECK(elapsed >= 150);
  CHECK(elapsed < 5000);
}

TEST_CASE("Sound: manual listen check (rising arpeggio)", "[.audio]") {
  lwbasic::SystemHost host;
  host.PlayTone(440, 300); // A4
  host.PlayTone(554, 300); // C#5
  host.PlayTone(659, 400); // E5
  SUCCEED("you should have heard three rising notes: A4, C#5, E5");
}

TEST_CASE("Sound: BEEP is 800 Hz for a quarter second", "[sound]") {
  // Reference: "sound the speaker at 800 Hz for one-quarter of a second".
  lwbasic::ScriptedHost host;
  lwtest::RunRowsWithInput("10 BEEP\n", {}, &host);
  REQUIRE(host.tones.size() == 1);
  CHECK(host.tones[0].first == 800);
  CHECK(host.tones[0].second == 250);
}

TEST_CASE("Sound: SOUND plays the requested tone", "[sound]") {
  lwbasic::ScriptedHost host;
  lwtest::RunRowsWithInput("10 SOUND 440, 18\n", {}, &host);
  REQUIRE(host.tones.size() == 1);
  CHECK(host.tones[0].first == 440);
  // 18 clock ticks at 18.2 per second = 989 ms.
  CHECK(host.tones[0].second == 989);
}

TEST_CASE("Sound: SOUND clamps the frequency and treats zero duration as silence", "[sound]") {
  lwbasic::ScriptedHost host;
  lwtest::RunRowsWithInput("10 SOUND 100000, 18\n"
                           "20 SOUND 10, 18\n"
                           "30 SOUND 440, 0\n",
                           {}, &host);
  REQUIRE(host.tones.size() == 2); // the zero-duration call plays nothing
  CHECK(host.tones[0].first == 32767);
  CHECK(host.tones[1].first == 37);
}

TEST_CASE("Sound: PLAY honours tempo, length, articulation and octave", "[sound]") {
  lwbasic::ScriptedHost host;
  lwtest::RunRowsWithInput("10 PLAY \"T120 L4 C\"\n", {}, &host);
  REQUIRE(host.tones.size() == 1);
  // O4 C is 523 Hz: octave 4 begins at C5 in scientific pitch, as the reference numbers it.
  CHECK(host.tones[0].first == 523);
  // A quarter note at T120 is 500 ms; MN plays seven eighths of it = 438 ms.
  CHECK(host.tones[0].second == 438);

  lwbasic::ScriptedHost octave;
  lwtest::RunRowsWithInput("10 PLAY \"O5 C\"\n", {}, &octave);
  REQUIRE(octave.tones.size() == 1);
  CHECK(octave.tones[0].first == 1047); // O5 C is one octave above O4 C

  lwbasic::ScriptedHost legato;
  lwtest::RunRowsWithInput("10 PLAY \"ML T120 L4 C\"\n", {}, &legato);
  REQUIRE(legato.tones.size() == 1);
  CHECK(legato.tones[0].second == 500); // ML plays the full period
}

TEST_CASE("Sound: PLAY rests, dots and sharps", "[sound]") {
  lwbasic::ScriptedHost host;
  lwtest::RunRowsWithInput("10 PLAY \"T120 L4 C P8 C8. C#4\"\n", {}, &host);
  REQUIRE(host.tones.size() == 4);
  CHECK(host.tones[0].first == 523);  // O4 C
  CHECK(host.tones[1].first == 0);    // rest
  CHECK(host.tones[1].second == 250); // P8 at T120: pauses are not articulated
  CHECK(host.tones[2].second == 328); // C8. = 250 * 1.5 * 7/8
  CHECK(host.tones[3].first == 554);  // C#5 = 554.4 Hz
}

TEST_CASE("Sound: a malformed PLAY string is an error", "[sound]") {
  CHECK(lwtest::RunErrorCode("10 PLAY \"Q\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PLAY \"O9 C\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 PLAY \"T10 C\"\n") == lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Sound: note frequencies follow the octave numbering", "[sound]") {
  using lwbasic::NoteFrequency;
  CHECK(NoteFrequency(0) == 0);    // rest
  CHECK(NoteFrequency(37) == 262); // middle C is the start of octave 3
  CHECK(NoteFrequency(46) == 440); // A4, the tuning reference
  CHECK(NoteFrequency(49) == 523); // C5, the start of octave 4
  CHECK(lwbasic::TicksToMilliseconds(18) == 989);
  CHECK(lwbasic::TicksToMilliseconds(0) == 0);
}

TEST_CASE("Sound: the Ode to Joy opening plays the right notes", "[sound]") {
  // The sample is samples/Beethoven.bas; the melody is repeated here so the test is
  // self-contained. T100 makes a quarter note 600 ms, and L4 makes every note a quarter.
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 PLAY \"T100 O3 L4 F# F# G A\"");
  repl.Process("20 PLAY \"T100 O3 L4 A G F# E\"");
  repl.Process("30 PLAY \"T100 O3 L4 D D E F#\"");
  repl.Process("40 END");
  repl.Process("RUN");
  REQUIRE(host.tones.size() == 12);
  // Equal temperament around A4 = 440: D4 294, E4 330, F#4 370, G4 392, A4 440. A few hertz of
  // slack covers the rounding the note table uses.
  const int expected[12] = {370, 370, 392, 440, 440, 392, 370, 330, 294, 294, 330, 370};
  for (std::size_t i = 0; i < 12; ++i) {
    INFO("note " << i);
    CHECK(std::abs(host.tones[i].first - expected[i]) <= 3);
    // MN articulation is the default, so each note sounds for 7/8 of its length: 525 of 600 ms.
    CHECK(host.tones[i].second == 525);
  }
}

TEST_CASE("Sound: the sixteen-bar Ode to Joy at speed", "[sound]") {
  // The sample is samples/Beethoven.bas; the melody is repeated here so the test stands alone.
  // Sixteen bars of four beats, with four of them ending on a half note: 60 notes in all.
  // T150 makes a quarter note 400 ms, and MN articulation sounds 7/8 of that: 350 ms, and 700 for
  // the halves.
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  const char* bars[] = {
      "10 PLAY \"T150 O3 L4 F# F# G A\"",  "20 PLAY \"T150 O3 L4 A G F# E\"",
      "30 PLAY \"T150 O3 L4 D D E F#\"",   "40 PLAY \"T150 O3 L4 F# E E2\"",
      "50 PLAY \"T150 O3 L4 F# F# G A\"",  "60 PLAY \"T150 O3 L4 A G F# E\"",
      "70 PLAY \"T150 O3 L4 D D E F#\"",   "80 PLAY \"T150 O3 L4 E D D2\"",
      "90 PLAY \"T150 O3 L4 E E F# D\"",   "100 PLAY \"T150 O3 L4 E F# G F#\"",
      "110 PLAY \"T150 O3 L4 D E F# G\"",  "120 PLAY \"T150 O3 L4 F# E D2\"",
      "130 PLAY \"T150 O3 L4 E E F# D\"",  "140 PLAY \"T150 O3 L4 E F# G F#\"",
      "150 PLAY \"T150 O3 L4 D E F# G\"",  "160 PLAY \"T150 O3 L4 F# E D2\"",
      "170 END",
  };
  for (const char* line : bars)
    repl.Process(line);
  repl.Process("RUN");
  REQUIRE(host.tones.size() == 60);
  // The opening phrase, the half note that closes bar 4, and the tonic the tune ends on.
  CHECK(std::abs(host.tones[0].first - 370) <= 3);  // F#4
  CHECK(std::abs(host.tones[1].first - 370) <= 3);
  CHECK(std::abs(host.tones[2].first - 392) <= 3);  // G4
  CHECK(std::abs(host.tones[3].first - 440) <= 3);  // A4
  // Bar 4 is F# E E2, and bars 1-3 hold four notes each, so the long E is the fourteenth tone.
  CHECK(std::abs(host.tones[14].first - 330) <= 3); // E4, the long note ending bar 4
  CHECK(host.tones[14].second == 700);              // a half note at T150
  CHECK(host.tones[59].second == 700);
  // Every note is either a quarter or a half, and most are quarters.
  int quarters = 0;
  for (const auto& tone : host.tones) {
    CHECK((tone.second == 350 || tone.second == 700));
    if (tone.second == 350)
      ++quarters;
  }
  CHECK(quarters == 56);
}

TEST_CASE("Sound: PLAY octaves follow the reference", "[sound]") {
  // The reference numbers octaves so that O3 contains middle C: O3 C is 261.6 Hz and O3 A is 440.
  // An off-by-one here transposes everything an octave without making any interval wrong, which
  // is hard to hear as a mistake but obviously wrong next to the score.
  std::vector<lwbasic::Tone> tones;
  std::string error;
  REQUIRE(lwbasic::ParsePlayString("O3 C", &tones, &error));
  REQUIRE(tones.size() == 1);
  CHECK(std::abs(tones[0].frequency_hz - 262) <= 1);
  tones.clear();
  REQUIRE(lwbasic::ParsePlayString("O3 A", &tones, &error));
  CHECK(std::abs(tones[0].frequency_hz - 440) <= 1);
  tones.clear();
  REQUIRE(lwbasic::ParsePlayString("O4 C", &tones, &error));
  CHECK(std::abs(tones[0].frequency_hz - 523) <= 1);
  tones.clear();
  REQUIRE(lwbasic::ParsePlayString("O6 B", &tones, &error));
  CHECK(std::abs(tones[0].frequency_hz - 3951) <= 4);
}

TEST_CASE("Sound: the synthesiser renders a pitched, enveloped tone", "[sound]") {
  using lwbasic::RenderTone;
  const std::vector<std::int16_t> tone = RenderTone(440, 100);
  CHECK(tone.size() == 4410); // 100 ms at 44.1 kHz
  // The envelope starts and ends near zero, so a note does not click at its edges.
  CHECK(std::abs(static_cast<int>(tone.front())) < 600);
  CHECK(std::abs(static_cast<int>(tone.back())) < 600);
  // It is audible but well inside 16-bit range.
  int peak = 0;
  for (const std::int16_t sample : tone)
    peak = std::max(peak, std::abs(static_cast<int>(sample)));
  CHECK(peak > 8000);
  CHECK(peak < 32767);
  // A 440 Hz tone crosses zero twice per cycle: about 88 times in 100 ms.
  int crossings = 0;
  for (std::size_t i = 1; i < tone.size(); ++i) {
    if ((tone[i - 1] < 0) != (tone[i] < 0))
      ++crossings;
  }
  CHECK(std::abs(crossings - 88) <= 3);
  // Silence is silent, and the right length.
  const std::vector<std::int16_t> silence = RenderTone(0, 50);
  CHECK(silence.size() == 2205);
  for (const std::int16_t sample : silence)
    CHECK(sample == 0);
}

TEST_CASE("Sound: the synthesiser has an ADSR shape and a steady sustain", "[sound]") {
  using lwbasic::RenderTone;
  const std::vector<std::int16_t> tone = RenderTone(440, 500);
  CHECK(tone.size() == 22050);
  // Attack: the first millisecond is much quieter than the body of the note.
  int early = 0;
  for (int i = 0; i < 44; ++i)
    early = std::max(early, std::abs(static_cast<int>(tone[static_cast<std::size_t>(i)])));
  int middle = 0;
  for (int i = 11025; i < 11025 + 441; ++i)
    middle = std::max(middle, std::abs(static_cast<int>(tone[static_cast<std::size_t>(i)])));
  CHECK(early * 4 < middle);
  // Sustain: two windows well after the decay have about the same level.
  auto peak_in = [&tone](int from, int count) {
    int value = 0;
    for (int i = from; i < from + count; ++i)
      value = std::max(value, std::abs(static_cast<int>(tone[static_cast<std::size_t>(i)])));
    return value;
  };
  const int first_window = peak_in(4410, 4410);
  const int third_window = peak_in(13230, 4410);
  CHECK(std::abs(first_window - third_window) < first_window / 6);
  // Nothing clips.
  for (const std::int16_t sample : tone)
    CHECK(sample != 32767);
}
