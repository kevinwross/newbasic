#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace lwbasic::screen {

// Indexed-colour framebuffer for the graphics modes (M12, phase 1).
//
// One byte per pixel holding a palette index; the screen palette maps indices to
// colours. Keeping this CPU-side means the headless backend can assert pixels in
// tests, and the D3D11 backend can upload the same bytes as a texture. Nothing here
// knows about windows or devices.

class Canvas {
public:
  Canvas() { Resize(320, 200); }
  Canvas(int width, int height) { Resize(width, height); }

  int Width() const { return width_; }
  int Height() const { return height_; }

  void Resize(int width, int height) {
    width_ = width < 1 ? 1 : width;
    height_ = height < 1 ? 1 : height;
    pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0);
    ClearClip();
  }

  void Clear(std::uint8_t colour = 0) { std::fill(pixels_.begin(), pixels_.end(), colour); }

  // VIEW clips drawing to a rectangle, inclusive of both corners; the default is the
  // whole canvas.
  void SetClip(int left, int top, int right, int bottom) {
    clip_left_ = left;
    clip_top_ = top;
    clip_right_ = right;
    clip_bottom_ = bottom;
  }
  void ClearClip() {
    clip_left_ = 0;
    clip_top_ = 0;
    clip_right_ = width_ - 1;
    clip_bottom_ = height_ - 1;
  }
  int ClipLeft() const { return clip_left_; }
  int ClipTop() const { return clip_top_; }
  int ClipRight() const { return clip_right_; }
  int ClipBottom() const { return clip_bottom_; }

  // Plotting clips silently, as the reference describes for out-of-range points.
  void SetPixel(int x, int y, std::uint8_t colour) {
    if (x < clip_left_ || y < clip_top_ || x > clip_right_ || y > clip_bottom_)
      return;
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
      return;
    pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
            static_cast<std::size_t>(x)] = colour;
  }

  // Out-of-range reads give 0 rather than throwing: POINT on a blank area of a
  // clipped program should not be able to fail a run.
  std::uint8_t Pixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
      return 0;
    return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                   static_cast<std::size_t>(x)];
  }

  const std::vector<std::uint8_t>& Pixels() const { return pixels_; }

  // Bresenham line, shared by LINE and DRAW (phase 2).
  void DrawLine(int x1, int y1, int x2, int y2, std::uint8_t colour) {
    DrawLineStyled(x1, y1, x2, y2, colour, 0xFFFF);
  }

  // LINE's `style` mask: each plotted point consumes the next circulating bit, and a
  // zero bit skips the store. The mask starts at the most significant bit, which is
  // the order the "style" examples read naturally in.
  void DrawLineStyled(int x1, int y1, int x2, int y2, std::uint8_t colour, int style) {
    unsigned mask = static_cast<unsigned>(style) & 0xFFFFu;
    if (mask == 0)
      return; // nothing would be plotted
    const auto plot = [&](int x, int y) {
      if ((mask & 0x8000u) != 0)
        SetPixel(x, y, colour);
      mask = ((mask << 1) | (mask >> 15)) & 0xFFFFu;
    };
    int dx = x2 - x1;
    int dy = y2 - y1;
    const int step_x = dx < 0 ? -1 : 1;
    const int step_y = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int x = x1;
    int y = y1;
    plot(x, y);
    if (dx >= dy) {
      int error = dx / 2;
      while (x != x2) {
        x += step_x;
        error -= dy;
        if (error < 0) {
          y += step_y;
          error += dx;
        }
        plot(x, y);
      }
    } else {
      int error = dy / 2;
      while (y != y2) {
        y += step_y;
        error -= dx;
        if (error < 0) {
          x += step_x;
          error += dy;
        }
        plot(x, y);
      }
    }
  }

  // LINE ... ,B: the four edges between two opposite corners.
  void DrawBox(int x1, int y1, int x2, int y2, std::uint8_t colour) {
    DrawLine(x1, y1, x2, y1, colour);
    DrawLine(x2, y1, x2, y2, colour);
    DrawLine(x2, y2, x1, y2, colour);
    DrawLine(x1, y2, x1, y1, colour);
  }

  // LINE ... ,BF: the same box with its interior filled.
  void FillBox(int x1, int y1, int x2, int y2, std::uint8_t colour) {
    const int left = x1 < x2 ? x1 : x2;
    const int right = x1 < x2 ? x2 : x1;
    const int top = y1 < y2 ? y1 : y2;
    const int bottom = y1 < y2 ? y2 : y1;
    for (int y = top; y <= bottom; ++y)
      for (int x = left; x <= right; ++x)
        SetPixel(x, y, colour);
  }

  // Ellipse or arc. Radii are in pixels; the caller converts the dialect's aspect
  // argument. Sampling rather than a midpoint algorithm: a step fine enough for the
  // largest radius leaves no gaps, and arcs, the negative-angle radius lines and any
  // aspect all fall out of the same loop.
  void DrawEllipse(int cx, int cy, int radius_x, int radius_y, double start_angle, double end_angle,
                   std::uint8_t colour) {
    const int rx = radius_x < 0 ? -radius_x : radius_x;
    const int ry = radius_y < 0 ? -radius_y : radius_y;
    if (rx == 0 || ry == 0) {
      DrawLine(cx, cy, cx + rx, cy + ry, colour);
      return;
    }
    // A negative angle draws a radius line and is then treated as positive.
    if (start_angle < 0.0) {
      const double positive = -start_angle;
      DrawLine(cx, cy, cx + static_cast<int>(std::lround(rx * std::cos(positive))),
               cy - static_cast<int>(std::lround(ry * std::sin(positive))), colour);
      start_angle = positive;
    }
    if (end_angle < 0.0) {
      const double positive = -end_angle;
      DrawLine(cx, cy, cx + static_cast<int>(std::lround(rx * std::cos(positive))),
               cy - static_cast<int>(std::lround(ry * std::sin(positive))), colour);
      end_angle = positive;
    }
    if (end_angle <= start_angle)
      end_angle = start_angle + 6.283185307179586;
    const int largest = rx > ry ? rx : ry;
    const int steps = (largest + 1) * 8; // eight samples per pixel of radius
    for (int i = 0; i <= steps; ++i) {
      const double angle = start_angle + (end_angle - start_angle) * static_cast<double>(i) / steps;
      SetPixel(cx + static_cast<int>(std::lround(rx * std::cos(angle))),
               cy - static_cast<int>(std::lround(ry * std::sin(angle))), colour);
    }
  }

  // PAINT: fill the region containing (x,y) up to `border`. A border of -1 means
  // "stop at any colour different from the seed", which is the reference default
  // when the border argument is omitted.
  void FloodFill(int x, int y, std::uint8_t fill, int border) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
      return;
    const std::uint8_t seed = Pixel(x, y);
    const int stop = border < 0 ? seed : border;
    std::vector<std::pair<int, int>> stack;
    stack.emplace_back(x, y);
    while (!stack.empty()) {
      const int px = stack.back().first;
      const int py = stack.back().second;
      stack.pop_back();
      if (px < 0 || py < 0 || px >= width_ || py >= height_)
        continue;
      const std::uint8_t colour = Pixel(px, py);
      if (colour == fill || colour == stop)
        continue;
      SetPixel(px, py, fill);
      stack.emplace_back(px + 1, py);
      stack.emplace_back(px - 1, py);
      stack.emplace_back(px, py + 1);
      stack.emplace_back(px, py - 1);
    }
  }

private:
  int width_ = 320;
  int height_ = 200;
  // VIEW's clipping rectangle, inclusive of both corners (M12 phase 3).
  int clip_left_ = 0;
  int clip_top_ = 0;
  int clip_right_ = 319;
  int clip_bottom_ = 199;
  std::vector<std::uint8_t> pixels_;
};

} // namespace lwbasic::screen
