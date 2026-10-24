// DRAWTEXT: a graphics-canvas text primitive. The font is the built-in 8x16
// CP437 bitmap, so these tests can assert exact pixels headlessly.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/errors.h"
#include "newbasic/screen/canvas.h"
#include "newbasic/screen/headless_screen.h"
#include "newbasic/vm.h"
#include "test_helpers.h"

using newbasic::screen::Canvas;
using newbasic::screen::HeadlessScreen;

namespace {

void RunOn(HeadlessScreen* screen, const std::string& source) {
  newbasic::Program program;
  newbasic::CompileError error;
  REQUIRE(lwtest::CompileSource(source, &program, &error));
  newbasic::Vm vm(*screen);
  vm.Load(std::move(program));
  const newbasic::RunResult result = vm.Run();
  INFO("error " << result.code << " in line " << result.line << ": " << result.message);
  REQUIRE(result.status != newbasic::RunResult::Status::Error);
}

} // namespace

TEST_CASE("DRAWTEXT: draws a CP437 glyph at a pixel anchor", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n20 DRAWTEXT (5, 5), CHR$(219), 3\n30 END\n");
  const Canvas& canvas = screen.Graphics();
  // CHR$(219) is the solid block: all 8x16 pixels are ink.
  CHECK(canvas.Pixel(5, 5) == 3);
  CHECK(canvas.Pixel(12, 20) == 3);
  CHECK(canvas.Pixel(13, 5) == 0);
  CHECK(canvas.Pixel(5, 21) == 0);
}

TEST_CASE("DRAWTEXT: the default colour is 1 and size is 1", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n20 DRAWTEXT (5, 5), CHR$(219)\n30 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(5, 5) == 1);
  CHECK(canvas.Pixel(12, 20) == 1);
  CHECK(canvas.Pixel(13, 20) == 0);
}

TEST_CASE("DRAWTEXT: size scales the glyph in both directions", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n20 DRAWTEXT (5, 5), CHR$(219), 2, 2\n30 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(5, 5) == 2);
  CHECK(canvas.Pixel(20, 36) == 2);
  CHECK(canvas.Pixel(21, 5) == 0);
  CHECK(canvas.Pixel(5, 37) == 0);
}

TEST_CASE("DRAWTEXT: an empty colour slot still accepts a size", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n20 DRAWTEXT (5, 5), CHR$(219), , 2\n30 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(5, 5) == 1);
  CHECK(canvas.Pixel(20, 36) == 1);
  CHECK(canvas.Pixel(21, 5) == 0);
}

TEST_CASE("DRAWTEXT: uses the CP437 glyph shapes, not just blocks", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n20 DRAWTEXT (0, 0), \"A\", 1\n30 END\n");
  const Canvas& canvas = screen.Graphics();
  // The first two rows of 'A' are blank; the third row is 0x38.
  CHECK(canvas.Pixel(3, 0) == 0);
  CHECK(canvas.Pixel(3, 2) == 1);
  CHECK(canvas.Pixel(0, 2) == 0);
}

TEST_CASE("DRAWTEXT: CR, LF, and CRLF each start one new line", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 DRAWTEXT (2, 2), CHR$(219)+CHR$(13)+CHR$(10)+CHR$(219), 1\n"
                 "30 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(2, 2) == 1);
  CHECK(canvas.Pixel(2, 17) == 1);
  CHECK(canvas.Pixel(2, 18) == 1);
  CHECK(canvas.Pixel(2, 33) == 1);
  CHECK(canvas.Pixel(2, 34) == 0);
}

TEST_CASE("DRAWTEXT: VIEW supplies the origin and clips the glyph", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 VIEW (10,10)-(30,30)\n"
                 "30 DRAWTEXT (-4,0), CHR$(219), 1\n"
                 "40 DRAWTEXT (31,0), CHR$(219), 1\n"
                 "50 END\n");
  const Canvas& canvas = screen.Graphics();
  // The first block maps to x=6..13; the view clips it to x=10..13.
  CHECK(canvas.Pixel(10, 10) == 1);
  CHECK(canvas.Pixel(13, 10) == 1);
  CHECK(canvas.Pixel(9, 10) == 0);
  CHECK(canvas.Pixel(14, 10) == 0);
  // The second block is entirely to the right of the view.
  CHECK(canvas.Pixel(30, 10) == 0);
}

TEST_CASE("DRAWTEXT: WINDOW maps the anchor and keeps glyphs upright", "[drawtext][graphics]") {
  HeadlessScreen up;
  RunOn(&up, "10 SCREEN 1\n"
             "20 WINDOW (0,0)-(100,100)\n"
             "30 DRAWTEXT (0,25), CHR$(219), 1\n"
             "40 END\n");
  // Default WINDOW has y up: y=25 maps to 149 of 0..199.
  CHECK(up.Graphics().Pixel(0, 149) == 1);
  CHECK(up.Graphics().Pixel(0, 50) == 0);

  HeadlessScreen down;
  RunOn(&down, "10 SCREEN 1\n"
               "20 WINDOW SCREEN (0,0)-(100,100)\n"
               "30 DRAWTEXT (0,25), CHR$(219), 1\n"
               "40 END\n");
  // WINDOW SCREEN has y down: y=25 maps to 50.
  CHECK(down.Graphics().Pixel(0, 50) == 1);
  CHECK(down.Graphics().Pixel(0, 149) == 0);
}

TEST_CASE("DRAWTEXT: STEP is relative to the previous text anchor", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 DRAWTEXT (10,10), \"A\", 1\n"
                 "30 DRAWTEXT STEP (0,20), CHR$(219), 2\n"
                 "40 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(10, 30) == 2);
  CHECK(canvas.Pixel(17, 45) == 2);
  CHECK(canvas.Pixel(10, 29) == 0);
}

TEST_CASE("DRAWTEXT: text is part of the canvas for GET and PUT", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 DRAWTEXT (10,10), CHR$(219), 1\n"
                 "30 GET (10,10)-(17,25), A%\n"
                 "40 PUT (50,50), A%\n"
                 "50 END\n");
  const Canvas& canvas = screen.Graphics();
  CHECK(canvas.Pixel(50, 50) == 1);
  CHECK(canvas.Pixel(57, 65) == 1);
}

TEST_CASE("DRAWTEXT: PCOPY snapshots text with the canvas", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 DRAWTEXT (10,10), CHR$(219), 3\n"
                 "30 PCOPY 0,1\n"
                 "40 CLS\n"
                 "50 PCOPY 1,0\n"
                 "60 END\n");
  CHECK(screen.Graphics().Pixel(10, 10) == 3);
}

TEST_CASE("DRAWTEXT: CLS 1 clears text inside the viewport", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 1\n"
                 "20 VIEW (10,10)-(30,30)\n"
                 "30 DRAWTEXT (0,0), CHR$(219), 1\n"
                 "40 CLS 1\n"
                 "50 END\n");
  CHECK(screen.Graphics().Pixel(10, 10) == 0);
}

TEST_CASE("DRAWTEXT: needs a graphics mode", "[drawtext][graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 0\n20 DRAWTEXT (0,0), \"X\"\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 3\n20 DRAWTEXT (0,0), \"X\"\n") ==
        newbasic::kErrIllegalFunctionCall);
}

TEST_CASE("DRAWTEXT: validates colour, size, and text type", "[drawtext][graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAWTEXT (0,0), \"X\", -1\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAWTEXT (0,0), \"X\", 99\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAWTEXT (0,0), \"X\", 1, 0\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAWTEXT (0,0), \"X\", 1, 9\n") ==
        newbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAWTEXT (0,0), 5\n") == newbasic::kErrTypeMismatch);
}

TEST_CASE("DRAWTEXT: accepts a 256-colour index in SCREEN 720", "[drawtext][graphics]") {
  HeadlessScreen screen;
  RunOn(&screen, "10 SCREEN 720\n20 DRAWTEXT (5,5), CHR$(219), 200\n30 END\n");
  CHECK(screen.Graphics().Pixel(5, 5) == 200);
}
