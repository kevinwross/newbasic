#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace newbasic {

// Sound support (M7): PLAY-string parsing and note frequencies, deliberately
// free of any audio API so it is unit-testable and silent. The Host performs
// the tone.
//
// Semantics taken from the GW-BASIC reference (docs/gw-basic):
//   BEEP          - the speaker at 800 Hz for one-quarter second.
//   SOUND f,d     - frequency 37..32767 Hz; duration in clock ticks at 18.2 per
//                   second, 0..65535. A duration of 0 silences; values below
//                   0.022 mean "until the next sound" (we approximate - see
//                   the VM's Sound op).
//   PLAY string   - musical macro language: A-G notes with #/+ sharp or - flat,
//                   an optional length suffix 1..64 and dots (each dot scales
//                   the duration by 3/2); O0-O6 octave (default 4, middle C at
//                   the start of octave 3); L1-L64 default length (default 4);
//                   T32-T255 tempo in quarter notes per minute (default 120);
//                   P or R pause; N0-N84 note numbers (0 = rest); MN normal
//                   (7/8 of the period), ML legato (full), MS staccato (3/4);
//                   MF/MB foreground/background - we always play in the
//                   foreground, so MB is accepted and treated as MF.
//     Xstring; (substring execution) is not implemented.

struct Tone {
  int frequency_hz = 0; // 0 means silence for the duration
  int duration_ms = 0;
};

// Note number 1..84 -> frequency, numbered as the reference numbers octaves: 1 is C of octave 0,
// 37 is middle C (the start of O3), and A4 is 46 = 440 Hz.
//
// The anchor used to be 58, which put A4 where note 58 sits only if octaves were counted one
// higher than the reference counts them, so every PLAY note sounded an octave low. Note 37 must
// give 261.6 Hz: that is what makes O3 C middle C, as the octave table in this file says.
inline int NoteFrequency(int note_number) {
  if (note_number <= 0)
    return 0;
  const double semitones = static_cast<double>(note_number) - 46.0;
  return static_cast<int>(440.0 * std::pow(2.0, semitones / 12.0) + 0.5);
}

// SOUND durations are in clock ticks at 18.2 ticks per second.
inline int TicksToMilliseconds(double ticks) {
  if (ticks <= 0.0)
    return 0;
  return static_cast<int>(ticks * 1000.0 / 18.2 + 0.5);
}

inline int ClampFrequency(int hertz) {
  if (hertz < 37)
    return 37;
  if (hertz > 32767)
    return 32767;
  return hertz;
}

// Renders one tone as 16-bit mono samples at 44100 Hz: two oscillators detuned by about a quarter
// of a hertz, a quarter-weight second harmonic, a vibrato that fades in after the attack, and an
// ADSR envelope.
//
// Each piece earns its place. Detuning the second oscillator gives the slow beating that stops a
// tone sounding synthetic. The harmonic adds body, since a bare sine has almost no energy above the
// fundamental. The vibrato waits 120 ms so the onset stays steady - singing vibrato arrives after
// the attack, not with it. The envelope is a 5 ms attack, a 10 ms decay to a 0.75 sustain and a
// 33 ms release, so notes start and end without a click.
//
// Kept here rather than in the host so it can be tested without an audio device: the tests count
// samples, check the envelope shape, and count zero crossings to confirm the pitch.
inline std::vector<std::int16_t> RenderTone(int frequency_hz, int duration_ms,
                                            int sample_rate = 44100) {
  const int frames = duration_ms <= 0 ? 0 : sample_rate * duration_ms / 1000;
  std::vector<std::int16_t> samples(static_cast<std::size_t>(frames > 0 ? frames : 0), 0);
  if (frames <= 0 || frequency_hz <= 0)
    return samples;
  const double two_pi = 6.283185307179586;
  const double step = two_pi * static_cast<double>(frequency_hz) / static_cast<double>(sample_rate);
  const int attack = (std::max)(1, sample_rate / 200); // 5 ms
  const int decay = (std::max)(1, sample_rate / 100);  // 10 ms
  const int release = (std::max)(1, sample_rate / 30); // 33 ms
  const double sustain = 0.75;
  const double vibrato_rate = 5.0;                    // Hz
  const double vibrato_depth = 0.003;                 // 0.3% of the frequency
  const int vibrato_delay = sample_rate * 120 / 1000; // steady onset
  const double vibrato_ramp = static_cast<double>(sample_rate) * 0.08;
  const double detune = 1.0015; // slow beating between the pair
  const double peak = 9000.0;   // 1.42 x 9000 stays inside 16 bits
  double phase = 0.0;
  double phase_detuned = 0.0;
  for (int i = 0; i < frames; ++i) {
    double vibrato = 0.0;
    if (i > vibrato_delay) {
      const double ramp = (std::min)(1.0, (i - vibrato_delay) / vibrato_ramp);
      vibrato = vibrato_depth * ramp * std::sin(two_pi * vibrato_rate * i / sample_rate);
    }
    const double ratio = 1.0 + vibrato;
    phase += step * ratio;
    phase_detuned += step * detune * ratio;
    const double tone =
        0.62 * std::sin(phase) + 0.62 * std::sin(phase_detuned) + 0.18 * std::sin(2.0 * phase);
    double envelope = sustain;
    if (i < attack) {
      envelope = static_cast<double>(i) / static_cast<double>(attack);
    } else if (i < attack + decay) {
      envelope = 1.0 - (1.0 - sustain) * static_cast<double>(i - attack) / decay;
    }
    const int from_end = frames - 1 - i;
    if (from_end < release)
      envelope *= static_cast<double>(from_end) / static_cast<double>(release);
    samples[static_cast<std::size_t>(i)] = static_cast<std::int16_t>(tone * envelope * peak);
  }
  return samples;
}

namespace detail {

inline int SemitoneForLetter(char letter) {
  switch (letter) {
  case 'C':
    return 0;
  case 'D':
    return 2;
  case 'E':
    return 4;
  case 'F':
    return 5;
  case 'G':
    return 7;
  case 'A':
    return 9;
  case 'B':
    return 11;
  default:
    return -1;
  }
}

inline int ReadNumber(const std::string& music, std::size_t* pos) {
  int value = 0;
  while (*pos < music.size() && music[*pos] >= '0' && music[*pos] <= '9') {
    value = value * 10 + (music[*pos] - '0');
    ++*pos;
  }
  return value;
}

} // namespace detail

// Parses a PLAY string into tones. Returns false with a short reason when a
// command is malformed or unsupported.
inline bool ParsePlayString(const std::string& music, std::vector<Tone>* tones,
                            std::string* error) {
  int octave = 4;
  int length = 4;
  int tempo = 120;
  // Articulation as a fraction of the note period: MN 7/8, ML 1, MS 3/4.
  int articulation_numerator = 7;
  int articulation_denominator = 8;

  std::size_t pos = 0;
  while (pos < music.size()) {
    const char raw = music[pos];
    if (raw == ' ' || raw == '\t' || raw == '\r' || raw == '\n') {
      ++pos;
      continue;
    }
    const char command = (raw >= 'a' && raw <= 'z') ? static_cast<char>(raw - 'a' + 'A') : raw;

    // Note letters A-G, with optional accidental, length suffix and dots.
    const int semitone = detail::SemitoneForLetter(command);
    if (semitone >= 0) {
      ++pos;
      int shift = 0;
      if (pos < music.size() && (music[pos] == '#' || music[pos] == '+')) {
        shift = 1;
        ++pos;
      } else if (pos < music.size() && music[pos] == '-') {
        shift = -1;
        ++pos;
      }
      int note_length = length;
      if (pos < music.size() && music[pos] >= '0' && music[pos] <= '9') {
        note_length = detail::ReadNumber(music, &pos);
      }
      int dots = 0;
      while (pos < music.size() && music[pos] == '.') {
        ++dots;
        ++pos;
      }
      if (note_length < 1 || note_length > 64) {
        if (error != nullptr)
          *error = "PLAY length must be 1..64";
        return false;
      }
      const int note_number = octave * 12 + semitone + shift + 1;
      if (note_number < 1 || note_number > 84) {
        if (error != nullptr)
          *error = "PLAY note is out of range";
        return false;
      }
      // Whole-note period from the tempo, then the note's share of it.
      double period = 240000.0 / static_cast<double>(tempo) / static_cast<double>(note_length);
      for (int d = 0; d < dots; ++d)
        period *= 1.5;
      period = period * articulation_numerator / articulation_denominator;
      Tone tone;
      tone.frequency_hz = NoteFrequency(note_number);
      tone.duration_ms = static_cast<int>(period + 0.5);
      tones->push_back(tone);
      continue;
    }

    switch (command) {
    case 'N': { // note number 0..84
      ++pos;
      const int note_number = detail::ReadNumber(music, &pos);
      int dots = 0;
      while (pos < music.size() && music[pos] == '.') {
        ++dots;
        ++pos;
      }
      if (note_number < 0 || note_number > 84) {
        if (error != nullptr)
          *error = "PLAY note number must be 0..84";
        return false;
      }
      double period = 240000.0 / static_cast<double>(tempo) / static_cast<double>(length);
      for (int d = 0; d < dots; ++d)
        period *= 1.5;
      period = period * articulation_numerator / articulation_denominator;
      Tone tone;
      tone.frequency_hz = NoteFrequency(note_number);
      tone.duration_ms = static_cast<int>(period + 0.5);
      tones->push_back(tone);
      break;
    }
    case 'O': { // octave 0..6
      ++pos;
      const int value = detail::ReadNumber(music, &pos);
      if (value < 0 || value > 6) {
        if (error != nullptr)
          *error = "PLAY octave must be 0..6";
        return false;
      }
      octave = value;
      break;
    }
    case 'L': { // default note length 1..64
      ++pos;
      const int value = detail::ReadNumber(music, &pos);
      if (value < 1 || value > 64) {
        if (error != nullptr)
          *error = "PLAY length must be 1..64";
        return false;
      }
      length = value;
      break;
    }
    case 'T': { // tempo 32..255 quarter notes per minute
      ++pos;
      const int value = detail::ReadNumber(music, &pos);
      if (value < 32 || value > 255) {
        if (error != nullptr)
          *error = "PLAY tempo must be 32..255";
        return false;
      }
      tempo = value;
      break;
    }
    case 'P':
    case 'R': { // pause
      ++pos;
      int pause_length = length;
      if (pos < music.size() && music[pos] >= '0' && music[pos] <= '9') {
        pause_length = detail::ReadNumber(music, &pos);
      }
      int dots = 0;
      while (pos < music.size() && music[pos] == '.') {
        ++dots;
        ++pos;
      }
      if (pause_length < 1 || pause_length > 64) {
        if (error != nullptr)
          *error = "PLAY pause must be 1..64";
        return false;
      }
      double period = 240000.0 / static_cast<double>(tempo) / static_cast<double>(pause_length);
      for (int d = 0; d < dots; ++d)
        period *= 1.5;
      Tone tone;
      tone.frequency_hz = 0;
      tone.duration_ms = static_cast<int>(period + 0.5);
      tones->push_back(tone);
      break;
    }
    case 'M': { // MF/MB background or foreground, MN/ML/MS articulation
      ++pos;
      if (pos >= music.size()) {
        if (error != nullptr)
          *error = "PLAY M needs a second letter";
        return false;
      }
      const char mode = (music[pos] >= 'a' && music[pos] <= 'z')
                            ? static_cast<char>(music[pos] - 'a' + 'A')
                            : music[pos];
      ++pos;
      if (mode == 'N') {
        articulation_numerator = 7;
        articulation_denominator = 8;
      } else if (mode == 'L') {
        articulation_numerator = 1;
        articulation_denominator = 1;
      } else if (mode == 'S') {
        articulation_numerator = 3;
        articulation_denominator = 4;
      } else if (mode != 'F' && mode != 'B') {
        if (error != nullptr)
          *error = "unsupported PLAY command";
        return false;
      }
      break;
    }
    default:
      if (error != nullptr)
        *error = "unsupported PLAY command";
      return false;
    }
  }
  return true;
}

} // namespace newbasic
