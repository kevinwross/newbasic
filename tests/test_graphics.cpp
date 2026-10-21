// M12 phase 1: the canvas, SCREEN modes, PSET/PRESET/POINT/LINE and CLS 1.
// The canvas is CPU-side, so all of this is assertable headlessly - the reason the
// design note called the geometry "the part that can be tested".
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

#include "lwbasic/errors.h"
#include "lwbasic/repl.h"
#include "lwbasic/screen/canvas.h"
#include "lwbasic/screen/headless_screen.h"
#include "lwbasic/screen/text_render.h"
#include "test_helpers.h"

using lwbasic::screen::Canvas;

TEST_CASE("Canvas: plotting, clipping and reads", "[graphics]") {
  Canvas canvas(10, 5);
  CHECK(canvas.Width() == 10);
  CHECK(canvas.Height() == 5);
  canvas.SetPixel(3, 2, 7);
  CHECK(canvas.Pixel(3, 2) == 7);
  // Clipped silently, and reads outside give 0 rather than failing.
  canvas.SetPixel(-1, 0, 9);
  canvas.SetPixel(10, 0, 9);
  canvas.SetPixel(0, 5, 9);
  CHECK(canvas.Pixel(-1, 0) == 0);
  CHECK(canvas.Pixel(100, 100) == 0);
  canvas.Clear();
  CHECK(canvas.Pixel(3, 2) == 0);
}

TEST_CASE("Canvas: Bresenham lines are symmetric and inclusive", "[graphics]") {
  Canvas forward(12, 12);
  Canvas backward(12, 12);
  forward.DrawLine(2, 3, 9, 8, 5);
  backward.DrawLine(9, 8, 2, 3, 5);
  // Both endpoints are always drawn.
  CHECK(forward.Pixel(2, 3) == 5);
  CHECK(forward.Pixel(9, 8) == 5);
  // The same set of pixels either way round.
  CHECK(forward.Pixels() == backward.Pixels());
  // A horizontal run covers every column.
  Canvas flat(8, 3);
  flat.DrawLine(1, 1, 6, 1, 3);
  for (int x = 1; x <= 6; ++x)
    CHECK(flat.Pixel(x, 1) == 3);
}

TEST_CASE("Graphics: SCREEN sets the mode and canvas size", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("SCREEN 1");
  CHECK(screen.GraphicsMode() == 1);
  CHECK(screen.Graphics().Width() == 320);
  CHECK(screen.Graphics().Height() == 200);
  repl.Process("SCREEN 2");
  CHECK(screen.Graphics().Width() == 640);
  CHECK(screen.Graphics().Height() == 200);
  repl.Process("SCREEN 9");
  CHECK(screen.Graphics().Width() == 640);
  CHECK(screen.Graphics().Height() == 350);
  // Every reference mode is provided now, so the rejection case has to be a mode that does not
  // exist at all.
  CHECK(lwtest::RunErrorCode("10 SCREEN 14\n") == lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: PSET, PRESET and POINT", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (10,20)");
  repl.Process("30 PSET (11,20),4");
  repl.Process("40 PRESET (10,20)");
  repl.Process("50 PRINT POINT(10,20);POINT(11,20)");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(10, 20) == 0); // PRESET cleared it
  CHECK(screen.Graphics().Pixel(11, 20) == 4);
  CHECK(screen.RenderRows()[0] == " 0  4");
}

TEST_CASE("Graphics: LINE draws between the endpoints", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 LINE (0,0)-(5,0),2");
  repl.Process("RUN");
  for (int x = 0; x <= 5; ++x)
    CHECK(screen.Graphics().Pixel(x, 0) == 2);
  CHECK(screen.Graphics().Pixel(6, 0) == 0);
}

TEST_CASE("Graphics: CLS 1 clears only the canvas", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (5,5),7");
  repl.Process("30 CLS 1");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(5, 5) == 0);
}

TEST_CASE("Graphics: PSET rejects an out-of-range colour", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PSET (1,1),99\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Canvas: an ellipse reaches its four extremes", "[graphics]") {
  Canvas canvas(101, 101);
  canvas.DrawEllipse(50, 50, 20, 20, 0.0, 6.283185307179586, 3);
  CHECK(canvas.Pixel(30, 50) == 3); // left
  CHECK(canvas.Pixel(70, 50) == 3); // right
  CHECK(canvas.Pixel(50, 30) == 3); // top (y grows downward)
  CHECK(canvas.Pixel(50, 70) == 3); // bottom
  Canvas tall(101, 201);
  tall.DrawEllipse(50, 100, 20, 60, 0.0, 6.283185307179586, 2);
  CHECK(tall.Pixel(30, 100) == 2);
  CHECK(tall.Pixel(50, 40) == 2);
  CHECK(tall.Pixel(50, 160) == 2);
}

TEST_CASE("Graphics: CIRCLE uses the mode aspect for a visual circle", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 CIRCLE (160,100),50,2");
  repl.Process("RUN");
  // SCREEN 1 is 320x200, so the default aspect is (4/3)*200/320 = 5/6, which is the
  // y radius over the x radius: the radius is measured in x pixels (50) and the y
  // radius is 50*(5/6) = 42. That gives a visual circle, because a 4:3 display of
  // 320x200 has pixels taller than they are wide.
  CHECK(screen.Graphics().Pixel(110, 100) == 2);
  CHECK(screen.Graphics().Pixel(210, 100) == 2);
  CHECK(screen.Graphics().Pixel(160, 100 - 42) == 2);
  CHECK(screen.Graphics().Pixel(160, 100 + 42) == 2);
  // Not at the taller radius the inverted reading of aspect produced.
  CHECK(screen.Graphics().Pixel(160, 100 - 60) == 0);
}

TEST_CASE("Graphics: CIRCLE honours start and end angles", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 CIRCLE (160,100),50,2,0,1.5707963267948966");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(210, 100) == 2);      // angle 0
  CHECK(screen.Graphics().Pixel(160, 100 - 42) == 2); // angle pi/2
  CHECK(screen.Graphics().Pixel(110, 100) == 0);      // angle pi: not drawn
}

TEST_CASE("Graphics: PAINT fills up to a border", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 LINE (10,10)-(30,10),2");
  repl.Process("30 LINE (10,30)-(30,30),2");
  repl.Process("40 LINE (10,10)-(10,30),2");
  repl.Process("50 LINE (30,10)-(30,30),2");
  repl.Process("60 PAINT (20,20),4,2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(20, 20) == 4); // inside: filled
  CHECK(screen.Graphics().Pixel(20, 10) == 2); // the border survives
  CHECK(screen.Graphics().Pixel(20, 5) == 0);  // outside: untouched
  CHECK(screen.Graphics().Pixel(5, 20) == 0);
}

TEST_CASE("Graphics: PAINT and CIRCLE reject out-of-range colours", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PAINT (5,5),99\n") ==
        lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 CIRCLE (5,5),3,99\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: DRAW movement starts at the centre and plots", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  // The current position persists between DRAW statements, so each one pins it with
  // an absolute M first.
  repl.Process("20 DRAW \"M160,100U10\"");
  repl.Process("30 DRAW \"M160,100C2R5\"");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(160, 95) == 1);  // up from the centre
  CHECK(screen.Graphics().Pixel(163, 100) == 2); // right, in colour 2
  // A line includes its start point, and the colour-2 line was drawn from the centre
  // afterwards, so the centre shows colour 2.
  CHECK(screen.Graphics().Pixel(160, 100) == 2);
}

TEST_CASE("Graphics: DRAW scale, angle and relative M", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  // S8 doubles each unit; A1 turns the movement 90 degrees and it persists.
  repl.Process("20 DRAW \"M160,100S8U10\"");
  repl.Process("30 DRAW \"A1U5\"");
  repl.Process("40 DRAW \"M+10,+10\"");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(160, 80) == 1); // U10 at scale 2 is 20 pixels up
  CHECK(screen.Graphics().Pixel(155, 80) == 1); // A1 turns "up" left; the scale persists
  CHECK(screen.Graphics().Pixel(160, 90) == 1); // M+10,+10 relative from (150,80)
}

TEST_CASE("Graphics: DRAW B moves without plotting, N returns home", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 DRAW \"BM10,10\"");
  repl.Process("30 DRAW \"U5\"");
  repl.Process("RUN");
  // B moved the start to (10,10) without drawing a line to it.
  CHECK(screen.Graphics().Pixel(160, 100) == 0);
  CHECK(screen.Graphics().Pixel(10, 5) == 1);

  // N plots, then puts the position back: the next DRAW starts at the centre again.
  lwbasic::screen::HeadlessScreen second;
  lwbasic::ScriptedHost second_host;
  lwbasic::Repl second_repl(second, &second_host);
  second_repl.Process("10 SCREEN 1");
  second_repl.Process("20 DRAW \"NU10\"");
  second_repl.Process("30 DRAW \"R5\"");
  second_repl.Process("RUN");
  CHECK(second.Graphics().Pixel(160, 90) == 1);  // the N move was drawn
  CHECK(second.Graphics().Pixel(165, 100) == 1); // and the position returned
}

TEST_CASE("Graphics: DRAW X executes a string variable", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  // The macro names the variable as written, suffix and all.
  repl.Process("20 A$=\"D10\"");
  repl.Process("30 DRAW \"M160,100XA$;\"");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(160, 110) == 1);
}

TEST_CASE("Graphics: DRAW rejects unknown commands", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAW \"Z10\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAW \"A9\"\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 DRAW \"S0\"\n") == lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: PALETTE remaps attributes and resets", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PALETTE 2,4"); // the reference's example: attribute 2 becomes red
  repl.Process("30 PALETTE 3,6"); // attribute 3 becomes brown
  repl.Process("RUN");
  CHECK(screen.Palette()[2] == 4);
  CHECK(screen.Palette()[3] == 6);
  // The RGB conversion follows the remapped palette.
  screen.Graphics().SetPixel(0, 0, 2);
  const std::vector<std::uint8_t> rgba =
      lwbasic::screen::CanvasToRgba(screen.Graphics(), screen.PaletteRgb());
  CHECK(rgba[0] == 170); // red: 170,0,0
  CHECK(rgba[1] == 0);
  CHECK(rgba[2] == 0);

  // No arguments restores the default palette.
  repl.Process("40 PALETTE");
  repl.Process("RUN");
  CHECK(screen.Palette()[2] == 2);
  CHECK(screen.Palette()[3] == 3);
}

TEST_CASE("Graphics: PALETTE rejects out-of-range values", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PALETTE 16,1\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PALETTE 1,64\n") == lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: PSET STEP is relative to the last point", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (100,50),2");
  repl.Process("30 PSET STEP(10,10),3");
  repl.Process("40 PSET STEP(-5,0),4");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(100, 50) == 2);
  CHECK(screen.Graphics().Pixel(110, 60) == 3);
  CHECK(screen.Graphics().Pixel(105, 60) == 4);
}

TEST_CASE("Graphics: LINE with an omitted first point continues from the last", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 LINE (10,10)-(20,10),2");
  // No first coordinate: from (20,10), the point the previous LINE ended at.
  repl.Process("30 LINE -(20,30),2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(20, 20) == 2); // down the vertical continuation
  CHECK(screen.Graphics().Pixel(20, 30) == 2); // its endpoint
  CHECK(screen.Graphics().Pixel(15, 10) == 2); // the first line is still there
}

TEST_CASE("Graphics: LINE STEP offsets both endpoints", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (50,50)");
  // The first STEP is relative to the last point, the second to the first coordinate.
  repl.Process("30 LINE STEP(10,0)-STEP(20,20),2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(60, 50) == 2); // first coordinate
  CHECK(screen.Graphics().Pixel(80, 70) == 2); // second, offset from the first
  CHECK(screen.Graphics().Pixel(70, 60) == 2); // a point on the way
}

TEST_CASE("Graphics: LINE with B draws a box and BF fills it", "[graphics]") {
  lwbasic::screen::HeadlessScreen box_screen;
  lwbasic::ScriptedHost box_host;
  lwbasic::Repl box_repl(box_screen, &box_host);
  box_repl.Process("10 SCREEN 1");
  box_repl.Process("20 LINE (10,10)-(30,30),2,B");
  box_repl.Process("RUN");
  CHECK(box_screen.Graphics().Pixel(10, 10) == 2); // corners and edges
  CHECK(box_screen.Graphics().Pixel(20, 10) == 2);
  CHECK(box_screen.Graphics().Pixel(10, 20) == 2);
  CHECK(box_screen.Graphics().Pixel(30, 30) == 2);
  CHECK(box_screen.Graphics().Pixel(20, 20) == 0); // interior is left alone

  lwbasic::screen::HeadlessScreen fill_screen;
  lwbasic::ScriptedHost fill_host;
  lwbasic::Repl fill_repl(fill_screen, &fill_host);
  fill_repl.Process("10 SCREEN 1");
  fill_repl.Process("20 LINE (10,10)-(30,30),3,BF");
  fill_repl.Process("RUN");
  CHECK(fill_screen.Graphics().Pixel(20, 20) == 3); // interior filled
  CHECK(fill_screen.Graphics().Pixel(10, 10) == 3);
}

TEST_CASE("Graphics: LINE accepts the empty colour slot before B", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  // ",,B" per the reference: the colour is omitted, so two commas are needed.
  repl.Process("20 LINE (5,5)-(25,25),,B");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(5, 5) == 1);   // default colour
  CHECK(screen.Graphics().Pixel(15, 15) == 0); // still a box, not filled
}

TEST_CASE("Graphics: each mode has its own default palette", "[graphics]") {
  // SCREEN 1, palette 0: 1 is green, 2 red, 3 brown (the reference's "yellow").
  lwbasic::screen::HeadlessScreen mode1;
  lwbasic::ScriptedHost host1;
  lwbasic::Repl repl1(mode1, &host1);
  repl1.Process("10 SCREEN 1");
  repl1.Process("RUN");
  CHECK(mode1.Palette()[1] == 2);
  CHECK(mode1.Palette()[2] == 4);
  CHECK(mode1.Palette()[3] == 6);
  // The RGBA conversion follows it: attribute 1 renders green.
  mode1.Graphics().SetPixel(0, 0, 1);
  const std::vector<std::uint8_t> rgba =
      lwbasic::screen::CanvasToRgba(mode1.Graphics(), mode1.PaletteRgb());
  CHECK(rgba[0] == 0);
  CHECK(rgba[1] == 170);
  CHECK(rgba[2] == 0);

  // SCREEN 2 is monochrome, so 1 is white.
  lwbasic::screen::HeadlessScreen mode2;
  lwbasic::ScriptedHost host2;
  lwbasic::Repl repl2(mode2, &host2);
  repl2.Process("10 SCREEN 2");
  repl2.Process("RUN");
  CHECK(mode2.Palette()[1] == 15);
}

TEST_CASE("Graphics: PALETTE overrides the mode default and SCREEN restores it", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PALETTE 1,5");
  repl.Process("RUN");
  CHECK(screen.Palette()[1] == 5); // the PALETTE statement wins

  // A later SCREEN statement resets the palette to that mode's default.
  repl.Process("30 SCREEN 1");
  repl.Process("RUN");
  CHECK(screen.Palette()[1] == 2);
}

TEST_CASE("Graphics: COLOR ,n selects the SCREEN 1 colour set", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 COLOR ,1"); // second set: 1 cyan, 2 magenta, 3 white
  repl.Process("RUN");
  CHECK(screen.Palette()[1] == 3);
  CHECK(screen.Palette()[2] == 5);
  CHECK(screen.Palette()[3] == 15);

  repl.Process("30 COLOR ,0"); // back to 1 green, 2 red, 3 brown
  repl.Process("RUN");
  CHECK(screen.Palette()[1] == 2);
  CHECK(screen.Palette()[3] == 6);

  // In a graphics mode the first argument is the background.
  repl.Process("40 COLOR 4,0");
  repl.Process("RUN");
  CHECK(screen.Palette()[0] == 4);
}

TEST_CASE("Graphics: WINDOW maps world coordinates onto the canvas", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 WINDOW (0,0)-(1,1)");
  repl.Process("30 PSET (0.5,0.5),2"); // the centre
  repl.Process("40 PSET (0,0),2");     // bottom left: y grows upward by default
  repl.Process("50 PSET (1,1),2");     // top right
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(160, 100) == 2);
  CHECK(screen.Graphics().Pixel(0, 199) == 2);
  CHECK(screen.Graphics().Pixel(319, 0) == 2);
}

TEST_CASE("Graphics: WINDOW SCREEN puts y downward", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 WINDOW SCREEN (0,0)-(1,1)");
  repl.Process("30 PSET (0,0),2"); // now the top left
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(0, 0) == 2);
}

TEST_CASE("Graphics: PMAP maps logical and physical coordinates both ways", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 WINDOW (0,0)-(100,100)");
  repl.Process("30 PRINT PMAP(0,0);PMAP(100,0);PMAP(0,1)");
  repl.Process("40 PRINT PMAP(319,2);PMAP(199,3)");
  repl.Process("RUN");
  // Logical 0 and 100 map to the x edges, and logical y 0 to the bottom row.
  CHECK(screen.RenderRows()[0] == " 0  319  199");
  // Physical 319 x is logical 100, and physical 199 y is logical 0.
  CHECK(screen.RenderRows()[1] == " 100  0");
}

TEST_CASE("Graphics: POINT reads a pixel and the current position", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (100,50),3");
  repl.Process("30 PRINT POINT(100,50);POINT(0);POINT(1)");
  repl.Process("40 PRINT POINT(400,400)");
  repl.Process("RUN");
  // The pixel colour, then POINT(n) as the current graphics position (x, y).
  CHECK(screen.RenderRows()[0] == " 3  100  50");
  // Out of range reads -1, per the reference.
  CHECK(screen.RenderRows()[1] == "-1");
}

TEST_CASE("Graphics: WINDOW with equal bounds is rejected", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 WINDOW (0,0)-(0,1)\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: VIEW offsets coordinates to the viewport origin", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (50,50)-(150,120)");
  repl.Process("30 PSET (0,0),2");   // the viewport's top-left corner
  repl.Process("40 PSET (10,20),3"); // offset from it
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(50, 50) == 2);
  CHECK(screen.Graphics().Pixel(60, 70) == 3);
  // Nothing is drawn outside the viewport, even though it is on the canvas.
  CHECK(screen.Graphics().Pixel(160, 70) == 0);
}

TEST_CASE("Graphics: VIEW clips drawing to the viewport", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (50,50)-(150,120)");
  // A line that runs well past the viewport: only the inside part survives.
  repl.Process("30 LINE (0,10)-(300,10),2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(50, 60) == 2);  // first column inside
  CHECK(screen.Graphics().Pixel(150, 60) == 2); // last column inside
  CHECK(screen.Graphics().Pixel(160, 60) == 0); // clipped
  CHECK(screen.Graphics().Pixel(40, 60) == 0);  // clipped the other way
}

TEST_CASE("Graphics: VIEW fills and borders the viewport", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (10,10)-(100,100),3,4");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(50, 50) == 3); // fill
  CHECK(screen.Graphics().Pixel(10, 10) == 4); // border corner
  CHECK(screen.Graphics().Pixel(5, 5) == 0);   // outside untouched
}

TEST_CASE("Graphics: VIEW with no arguments restores the whole canvas", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (50,50)-(150,120)");
  repl.Process("30 VIEW");
  repl.Process("40 PSET (5,5),2");
  repl.Process("50 PSET (200,150),2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(5, 5) == 2); // back to canvas coordinates
  CHECK(screen.Graphics().Pixel(200, 150) == 2);
}

TEST_CASE("Graphics: VIEW and WINDOW compose", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (100,50)-(200,150)");
  repl.Process("30 WINDOW (0,0)-(10,10)");
  // The middle of the world is the middle of the viewport.
  repl.Process("40 PSET (5,5),2");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(150, 100) == 2);
  CHECK(screen.Graphics().Pixel(160, 100) == 0);
}

TEST_CASE("Graphics: GET captures a rectangle and PUT copies it", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 LINE (10,10)-(19,19),2,B"); // a 10x10 outline
  repl.Process("30 GET (10,10)-(19,19), A%");
  repl.Process("40 PUT (50,60), A%");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(50, 60) == 2); // copied corner
  CHECK(screen.Graphics().Pixel(59, 69) == 2); // copied far corner
  CHECK(screen.Graphics().Pixel(55, 65) == 0); // interior stays empty
  CHECK(screen.Graphics().Pixel(10, 10) == 2); // the original is untouched
}

TEST_CASE("Graphics: PUT actions combine with what is already there", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (0,0),3");   // one pixel of an otherwise empty 2x2 block
  repl.Process("25 PSET (10,10),3"); // the same colour at the destination
  repl.Process("30 GET (0,0)-(1,1), A%");
  repl.Process("40 PUT (10,10), A%, XOR"); // 3 XOR 3 clears both
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(10, 10) == 0);
  CHECK(screen.Graphics().Pixel(11, 11) == 0);

  // PRESET clears the destination.
  repl.Process("50 PUT (0,0), A%, PRESET");
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(0, 0) == 0);
}

TEST_CASE("Graphics: PUT without a GET is Illegal function call", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PUT (0,0), B%\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: GET and PUT respect the viewport", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 VIEW (40,30)-(140,130)");
  repl.Process("30 PSET (0,0),2"); // the viewport origin, canvas (40,30)
  repl.Process("40 GET (0,0)-(3,3), A%");
  repl.Process("50 PUT (10,10), A%"); // canvas (50,40)
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(50, 40) == 2);
}

TEST_CASE("Graphics: LINE style masks pattern the line", "[graphics]") {
  lwbasic::screen::HeadlessScreen solid_screen;
  lwbasic::ScriptedHost solid_host;
  lwbasic::Repl solid(solid_screen, &solid_host);
  solid.Process("10 SCREEN 1");
  solid.Process("20 LINE (0,0)-(7,0),2,,&HFFFF"); // every point
  solid.Process("RUN");
  for (int x = 0; x < 8; ++x)
    CHECK(solid_screen.Graphics().Pixel(x, 0) == 2);

  lwbasic::screen::HeadlessScreen dashed_screen;
  lwbasic::ScriptedHost dashed_host;
  lwbasic::Repl dashed(dashed_screen, &dashed_host);
  dashed.Process("10 SCREEN 1");
  dashed.Process("20 LINE (0,0)-(7,0),2,,&HAAAA"); // every other point, from bit 15
  dashed.Process("RUN");
  CHECK(dashed_screen.Graphics().Pixel(0, 0) == 2);
  CHECK(dashed_screen.Graphics().Pixel(1, 0) == 0);
  CHECK(dashed_screen.Graphics().Pixel(2, 0) == 2);
  CHECK(dashed_screen.Graphics().Pixel(3, 0) == 0);

  // A zero mask draws nothing at all.
  lwbasic::screen::HeadlessScreen blank_screen;
  lwbasic::ScriptedHost blank_host;
  lwbasic::Repl blank(blank_screen, &blank_host);
  blank.Process("10 SCREEN 1");
  blank.Process("20 LINE (0,0)-(20,0),2,,0");
  blank.Process("RUN");
  CHECK(blank_screen.Graphics().Pixel(0, 0) == 0);
  CHECK(blank_screen.Graphics().Pixel(10, 0) == 0);
}

TEST_CASE("Graphics: LINE style must be a constant", "[graphics]") {
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 A=1\n30 LINE (0,0)-(10,0),2,,A\n") != 0);
}

TEST_CASE("Graphics: PCOPY snapshots and restores the canvas", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 1");
  repl.Process("20 PSET (10,10),2");
  repl.Process("30 PCOPY 0,1"); // snapshot the canvas into page 1
  repl.Process("40 CLS");       // wipe the visible canvas
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(10, 10) == 0); // gone
  repl.Process("50 PCOPY 1,0");                // restore it
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(10, 10) == 2); // back
}

TEST_CASE("Graphics: PCOPY rejects bad pages", "[graphics]") {
  // Page 9 does not exist, and page 5 has never been filled.
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PCOPY 9,1\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PCOPY 5,0\n") == lwbasic::kErrIllegalFunctionCall);
  // Copying a page onto itself is harmless.
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PCOPY 0,0\n") == 0);
}

TEST_CASE("Graphics: SCREEN 9 is 640x350 with all sixteen colours", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 9");
  repl.Process("20 PSET (639,349),7");
  repl.Process("30 PSET (0,0),15");
  repl.Process("RUN");
  CHECK(screen.Graphics().Width() == 640);
  CHECK(screen.Graphics().Height() == 350);
  // The whole corner is reachable, and the palette is the identity (16 colours).
  CHECK(screen.Graphics().Pixel(639, 349) == 7);
  CHECK(screen.Graphics().Pixel(0, 0) == 15);
  CHECK(screen.Palette()[7] == 7);
  CHECK(screen.Palette()[15] == 15);
  // PALETTE still remaps in this mode.
  repl.Process("40 PALETTE 7,2");
  repl.Process("RUN");
  CHECK(screen.Palette()[7] == 2);
}

TEST_CASE("Graphics: SCREEN 9 CIRCLE uses its own aspect", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 9");
  repl.Process("20 CIRCLE (320,175),100,4");
  repl.Process("RUN");
  // (4/3) * 350 / 640 = 0.729, so the y radius is 100 * 0.729 = 73.
  CHECK(screen.Graphics().Pixel(220, 175) == 4);      // left
  CHECK(screen.Graphics().Pixel(420, 175) == 4);      // right
  CHECK(screen.Graphics().Pixel(320, 175 - 73) == 4); // top
  CHECK(screen.Graphics().Pixel(320, 175 + 73) == 4); // bottom
  CHECK(screen.Graphics().Pixel(320, 175 - 100) == 0);
}

TEST_CASE("Graphics: SCREEN 9 fills the screen and clips at the edges", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 9");
  repl.Process("20 LINE (0,0)-(639,349),3,B");
  repl.Process("30 LINE (-50,-50)-(700,400),2"); // runs off every edge
  repl.Process("RUN");
  CHECK(screen.Graphics().Pixel(0, 0) == 3);
  CHECK(screen.Graphics().Pixel(639, 349) == 3);
  // The over-long line was clipped to the canvas: it starts above and left of it, so it
  // must enter through the top edge and leave through the bottom.
  int bottom_hits = 0;
  int top_hits = 0;
  for (int x = 0; x < 640; ++x) {
    if (screen.Graphics().Pixel(x, 349) == 2)
      ++bottom_hits;
    if (screen.Graphics().Pixel(x, 0) == 2)
      ++top_hits;
  }
  CHECK(top_hits > 0);    // entered through the top edge
  CHECK(bottom_hits > 0); // left through the bottom edge
}

TEST_CASE("Graphics: SCREEN 720 is a wide 256-colour mode", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 720");
  repl.Process("RUN");
  // 1280x720 with a 160x45 text grid, which is 8x16 cells exactly.
  CHECK(screen.Graphics().Width() == 1280);
  CHECK(screen.Graphics().Height() == 720);
  CHECK(screen.Width() == 160);
  CHECK(screen.Height() == 45);
  CHECK(screen.DisplayAspect() > 1.7); // 16:9, not 4:3
  // The far corner of the grid is addressable, so the grid really is 160x45.
  screen.PutCharAt(44, 159, L'X');
  CHECK(screen.CellAt(44, 159) == L'X');
  // Colours up to 255 are legal here ...
  CHECK(lwtest::RunErrorCode("10 SCREEN 720\n20 PSET (10,10),200\n") == 0);
  // ... and still only 0-15 in a compatible mode.
  CHECK(lwtest::RunErrorCode("10 SCREEN 1\n20 PSET (1,1),200\n") ==
        lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Graphics: leaving SCREEN 720 restores the standard grid", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 720");
  repl.Process("20 SCREEN 1");
  repl.Process("RUN");
  CHECK(screen.Width() == 80);
  CHECK(screen.Height() == 25);
  CHECK(screen.Graphics().Width() == 320);
}

TEST_CASE("Graphics: the 256-colour palette is programmable", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 720");
  // PALETTE attribute, 18-bit colour: six bits per channel, red in the low bits.
  repl.Process("20 PALETTE 200, 63");         // pure red
  repl.Process("30 PALETTE 201, 63 * 256");   // pure green
  repl.Process("40 PALETTE 202, 63 * 65536"); // pure blue
  repl.Process("RUN");
  const auto& palette = screen.PaletteRgb();
  CHECK(palette[200][0] == 255);
  CHECK(palette[200][1] == 0);
  CHECK(palette[201][1] == 255);
  CHECK(palette[202][2] == 255);
}

TEST_CASE("Graphics: the sixteen-colour and VGA modes have the right canvas", "[graphics]") {
  struct Mode {
    int number;
    int width;
    int height;
    bool many_colours;
  };
  const Mode modes[] = {
      {7, 320, 200, false}, {8, 640, 200, false}, {12, 640, 480, false}, {13, 320, 200, true}};
  for (const Mode& mode : modes) {
    lwbasic::screen::HeadlessScreen screen;
    lwbasic::ScriptedHost host;
    lwbasic::Repl repl(screen, &host);
    repl.Process("10 SCREEN " + std::to_string(mode.number));
    repl.Process("RUN");
    CHECK(screen.Graphics().Width() == mode.width);
    CHECK(screen.Graphics().Height() == mode.height);
    CHECK(screen.Width() == 80); // the compatible modes keep the standard text grid
    CHECK(screen.Height() == 25);
    CHECK(screen.DisplayAspect() < 1.4); // 4:3, not the wide mode's 16:9
    // SCREEN 13 has 256 colours; the others have the sixteen CGA ones.
    CHECK(lwtest::RunErrorCode("10 SCREEN " + std::to_string(mode.number) +
                               "\n20 PSET (1,1),200\n") ==
          (mode.many_colours ? 0 : lwbasic::kErrIllegalFunctionCall));
  }
}

TEST_CASE("Graphics: text uses the whole wide grid in SCREEN 720", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 720");
  repl.Process("20 CLS");
  repl.Process("30 LOCATE 45, 158");
  repl.Process("40 PRINT \"Z\";");
  repl.Process("RUN");
  // Column 158 of a 160-wide grid is addressable and was written. Writing in the last
  // column would advance the cursor past the edge, wrap and scroll.
  CHECK(screen.CellAt(44, 157) == L'Z');
  // Clamping: asking for row and column past the end stays inside the grid.
  repl.Process("50 LOCATE 99, 999");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 44);
  CHECK(screen.CursorCol() == 159);
  // And the text window covers the whole grid, not the 80x25 default.
  repl.Process("60 VIEW PRINT");
  repl.Process("RUN");
  CHECK(screen.CellAt(0, 0) == L' '); // still inside the window, so untouched by CLS
}

TEST_CASE("Graphics: the text window follows the wide grid", "[graphics]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 SCREEN 720");
  repl.Process("20 LOCATE 45, 1");
  repl.Process("30 PRINT \"BOTTOM\";");
  repl.Process("40 PRINT");
  repl.Process("RUN");
  // Printing a newline on the last row scrolls the window: the bottom row is cleared and
  // the text moved up one, which only works if the window really ends at row 45.
  CHECK(screen.CellAt(44, 0) == L' ');
  CHECK(screen.CellAt(43, 0) == L'B');
}

TEST_CASE("Screen: text mode has a usable RGB palette", "[screen]") {
  // Text mode never calls SetGraphicsMode, so the constructor has to fill the RGB palette.
  // Without it every attribute resolved to black, which made a text-mode view dump blank
  // while the window still looked right, because the renderer reads the CGA table directly.
  lwbasic::screen::HeadlessScreen screen;
  const auto& palette = screen.PaletteRgb();
  CHECK(palette[7][0] == 170); // the default attribute is light grey
  CHECK(palette[7][1] == 170);
  CHECK(palette[7][2] == 170);
  CHECK(palette[4][0] == 170); // red, from the CGA table
  CHECK(palette[4][1] == 0);
  CHECK(palette[0][0] == 0); // black is still black
}

namespace {
// A screen that only counts presents, used to pin the difference between the throttled
// PresentIfDirty and the forced PresentNow.
class CountingScreen : public lwbasic::screen::Screen {
public:
  void Present() override { ++presents; }
  void EnterFullScreen() override {}
  void ExitFullScreen() override {}
  std::wstring ReadLine() override { return {}; }
  bool TryGetKey(wchar_t*) override { return false; }
  int presents = 0;
};
} // namespace

TEST_CASE("Screen: PresentIfDirty is throttled, PresentNow always presents", "[screen]") {
  CountingScreen screen;
  screen.PutCharAt(0, 0, L'A');
  screen.PresentIfDirty();
  CHECK(screen.presents == 1);

  // Drawing again immediately: the throttle suppresses the frame. This is what used to
  // swallow the REPL prompt and put the echoed input on the wrong line.
  screen.PutCharAt(0, 0, L'B');
  screen.PresentIfDirty();
  CHECK(screen.presents == 1);

  // A blocking read needs a frame regardless of the throttle.
  screen.PresentNow();
  CHECK(screen.presents == 2);

  // PresentNow clears the dirty flag as well, so a following call does nothing.
  screen.PresentNow();
  CHECK(screen.presents == 3);
  screen.PresentIfDirty();
  CHECK(screen.presents == 3);
}

TEST_CASE("Screen: EchoInputLine records typed input in the grid", "[screen]") {
  // A repainting backend draws the grid, so input that only exists in the console echo is
  // wiped by the next paint. EchoInputLine puts it in the model.
  lwbasic::screen::HeadlessScreen screen;
  screen.EchoInputLine(L"10 PRINT 2+3");
  CHECK(screen.CellAt(0, 0) == L'1');
  CHECK(screen.CellAt(0, 3) == L'P');
  CHECK(screen.CursorRow() == 1);
  CHECK(screen.CursorCol() == 0);
  // An empty line still advances, which is what pressing Enter on its own does.
  screen.EchoInputLine(L"");
  CHECK(screen.CursorRow() == 2);
}

namespace {
// A key source that replays a fixed string, for testing the line editor without a window.
class ScriptedKeys : public lwbasic::screen::KeySource {
public:
  explicit ScriptedKeys(std::wstring keys) : keys_(std::move(keys)) {}
  bool WaitKey(wchar_t* key) override {
    if (next_ >= keys_.size())
      return false;
    *key = keys_[next_++];
    return true;
  }

private:
  std::wstring keys_;
  std::size_t next_ = 0;
};
} // namespace

TEST_CASE("Input: the line editor echoes what is typed", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  ScriptedKeys keys(L"10 PRINT 2+3\r");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line == L"10 PRINT 2+3");
  // The characters are in the grid, not only in the returned string, so a repaint keeps
  // them on screen.
  CHECK(screen.CellAt(0, 0) == L'1');
  CHECK(screen.CellAt(0, 3) == L'P');
  // Enter moves past the line.
  CHECK(screen.CursorRow() == 1);
  CHECK(screen.CursorCol() == 0);
}

TEST_CASE("Input: backspace removes the character and its cell", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  // Split the literal: \x08C would parse as the single hex escape 0x08C.
  ScriptedKeys keys(L"AB"
                    L"\x08"
                    L"C\r");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line == L"AC");
  CHECK(screen.CellAt(0, 0) == L'A');
  CHECK(screen.CellAt(0, 1) == L'C');
  CHECK(screen.CellAt(0, 2) == L' ');
  // Backspace at the start of the line is harmless.
  ScriptedKeys more(L"\x08X\r");
  std::wstring second;
  REQUIRE(screen.EditLine(more, &second));
  CHECK(second == L"X");
}

TEST_CASE("Input: DEL also acts as backspace", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  // xterm and many other terminals send 0x7F for the Backspace key rather than
  // the 0x08 that the Win32 console reports.
  ScriptedKeys keys(L"AB"
                    L"\x7F"
                    L"C\r");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line == L"AC");
  CHECK(screen.CellAt(0, 0) == L'A');
  CHECK(screen.CellAt(0, 1) == L'C');
  CHECK(screen.CellAt(0, 2) == L' ');
}

TEST_CASE("Input: Escape abandons the line, end of input reports itself", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  ScriptedKeys keys(L"AB\x1b");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line.empty());

  // A source that ends mid-line must be reported, so a caller can stop rather than spin.
  ScriptedKeys short_source(L"unfinished");
  std::wstring partial;
  CHECK_FALSE(screen.EditLine(short_source, &partial));
}

TEST_CASE("Input: a long line wraps instead of running off the grid", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  ScriptedKeys keys(std::wstring(100, L'x') + L"\r");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line.size() == 100);
  CHECK(screen.CursorRow() >= 1); // wrapped past the first row
  CHECK(screen.CellAt(0, 0) == L'x');
  CHECK(screen.CellAt(0, 79) == L'x');
}

namespace {
// A screen that reports one break request, to exercise the interrupt path without a window.
class BreakOnceScreen : public lwbasic::screen::HeadlessScreen {
public:
  bool TakeBreak() override {
    if (!armed)
      return false;
    armed = false;
    return true;
  }
  bool armed = true;
};
} // namespace

TEST_CASE("Input: a break request stops a running program", "[input]") {
  BreakOnceScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  // An endless loop: it can only end by being interrupted.
  repl.Process("10 GOTO 10");
  const lwbasic::RunResult result = repl.Process("RUN");
  CHECK(result.status == lwbasic::RunResult::Status::Break);
  // The request was consumed, so a second run is not broken immediately.
  CHECK_FALSE(screen.TakeBreak());
}

namespace {
// A screen with a pointing device, so the mouse functions can be tested without a window.
class ClickScreen : public lwbasic::screen::HeadlessScreen {
public:
  bool TakeMouseClick(lwbasic::screen::Screen::MouseClick* click) override {
    if (clicks.empty())
      return false;
    *click = clicks.front();
    clicks.erase(clicks.begin());
    return true;
  }
  std::vector<MouseClick> clicks;
};
} // namespace

TEST_CASE("Input: MOUSEB reports a click and MOUSEX and MOUSEY follow it", "[input]") {
  ClickScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  // CLS before each print so the assertion always sees the latest output on row 0.
  repl.Process("5 CLS");

  // Nothing pending: the button is 0 and the position is still the origin.
  repl.Process("10 PRINT MOUSEB; MOUSEX; MOUSEY");
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == " 0  0  0");

  // A left click, then the coordinates of that click.
  lwbasic::screen::Screen::MouseClick left;
  left.x = 100;
  left.y = 50;
  left.button = 1;
  screen.clicks.push_back(left);
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == " 1  100  50");

  // Consumed: the button reads 0 again but the position persists.
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == " 0  100  50");

  // A right click reports 2, and MOUSE(n) reaches the same fields.
  lwbasic::screen::Screen::MouseClick right;
  right.x = 7;
  right.y = 9;
  right.button = 2;
  screen.clicks.push_back(right);
  repl.Process("20 PRINT MOUSEB; MOUSE(1); MOUSE(2)");
  repl.Process("RUN");
  CHECK(screen.RenderRows()[0] == " 2  7  9");
}

TEST_CASE("Input: the mouse functions are safe without a pointing device", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);
  repl.Process("10 PRINT MOUSEB; MOUSEX; MOUSEY");
  const lwbasic::RunResult result = repl.Process("RUN");
  CHECK(result.status == lwbasic::RunResult::Status::Done);
  CHECK(screen.RenderRows()[0] == " 0  0  0");
  // MOUSE(n) rejects a field it does not have.
  CHECK(lwtest::RunErrorCode("10 X = MOUSE(3)\n") == lwbasic::kErrIllegalFunctionCall);
}

TEST_CASE("Lexer: the mouse names are reserved words", "[input]") {
  // The front end only treats a name as a function when the lexer reports it as a keyword,
  // so this is the gate that makes MOUSE(n) parse as a call rather than an array.
  CHECK(lwbasic::Lexer::IsKeyword("MOUSE"));
  CHECK(lwbasic::Lexer::IsKeyword("MOUSEB"));
  CHECK(lwbasic::Lexer::IsKeyword("MOUSEX"));
  CHECK(lwbasic::Lexer::IsKeyword("MOUSEY"));
  CHECK(lwbasic::Lexer::IsKeyword("POINT"));
  CHECK_FALSE(lwbasic::Lexer::IsKeyword("MOUS"));
}

TEST_CASE("Input: the full LOCATE syntax", "[input]") {
  lwbasic::screen::HeadlessScreen screen;
  lwbasic::ScriptedHost host;
  lwbasic::Repl repl(screen, &host);

  // Both arguments.
  repl.Process("10 LOCATE 3, 5");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 2);
  CHECK(screen.CursorCol() == 4);

  // Row omitted: the column moves, the row stays where it was.
  repl.Process("20 LOCATE , 10");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 2);
  CHECK(screen.CursorCol() == 9);

  // Column omitted: one argument and no comma.
  repl.Process("30 LOCATE 7");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 6);
  CHECK(screen.CursorCol() == 9);

  // The cursor flag is honoured, and omitted arguments leave it alone.
  repl.Process("40 LOCATE , , 0");
  repl.Process("RUN");
  CHECK_FALSE(screen.CursorVisible());
  CHECK(screen.CursorRow() == 6);
  repl.Process("50 LOCATE , , 1");
  repl.Process("RUN");
  CHECK(screen.CursorVisible());

  // Scan lines are accepted and ignored, and the position still applies.
  repl.Process("60 LOCATE 2, 3, 1, 6, 7");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 1);
  CHECK(screen.CursorCol() == 2);
  CHECK(screen.CursorVisible());

  // A bare LOCATE is legal and leaves everything as it was.
  repl.Process("70 LOCATE");
  repl.Process("RUN");
  CHECK(screen.CursorRow() == 1);
  CHECK(screen.CursorCol() == 2);
  CHECK(screen.CursorVisible());
}

TEST_CASE("Input: reading a line turns the cursor back on", "[input]") {
  // A program may hide the cursor while drawing; waiting for input with no caret leaves the
  // user nowhere to look, so the editor turns it back on.
  lwbasic::screen::HeadlessScreen screen;
  screen.SetCursorVisible(false);
  CHECK_FALSE(screen.CursorVisible());
  ScriptedKeys keys(L"hi\r");
  std::wstring line;
  REQUIRE(screen.EditLine(keys, &line));
  CHECK(line == L"hi");
  CHECK(screen.CursorVisible());
}

TEST_CASE("SCREEN modes 3 to 11 are accepted", "[graphics]") {
  // Mode 3 is 80x25 text on a colour card: printing still works and there is no canvas.
  CHECK(lwtest::Row0("10 SCREEN 3\n20 PRINT \"T\";\n") == "T");

  // Each graphics mode is checked at its far corner, which is what proves the canvas size.
  CHECK(lwtest::Row0("10 SCREEN 4\n20 PSET (319, 199), 3\n"
                     "30 IF POINT(319, 199) = 3 THEN PRINT \"OK\";\n") == "OK");
  CHECK(lwtest::Row0("10 SCREEN 5\n20 PSET (319, 199), 3\n"
                     "30 IF POINT(319, 199) = 3 THEN PRINT \"OK\";\n") == "OK");
  CHECK(lwtest::Row0("10 SCREEN 6\n20 PSET (639, 199), 1\n"
                     "30 IF POINT(639, 199) = 1 THEN PRINT \"OK\";\n") == "OK");
  CHECK(lwtest::Row0("10 SCREEN 10\n20 PSET (639, 349), 1\n"
                     "30 IF POINT(639, 349) = 1 THEN PRINT \"OK\";\n") == "OK");
  CHECK(lwtest::Row0("10 SCREEN 11\n20 PSET (639, 479), 1\n"
                     "30 IF POINT(639, 479) = 1 THEN PRINT \"OK\";\n") == "OK");

  // A mode outside the reference is still refused.
  CHECK(lwtest::RunErrorCode("10 SCREEN 14\n") == lwbasic::kErrIllegalFunctionCall);
  CHECK(lwtest::RunErrorCode("10 SCREEN -1\n") == lwbasic::kErrIllegalFunctionCall);
}
