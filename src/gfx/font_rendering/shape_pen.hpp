#pragma once

#include "blend2d.h"
#include "editor/highlight.hpp"
#include "utils/region.hpp"
#include <array>
#include <variant>
#include <mdspan>
#include <set>

namespace shape {

enum Weight { None, Light, Heavy, Double };
enum Side { Up, Down, Left, Right };
enum QuadDir { UpLeft, UpRight, DownLeft, DownRight };

struct HLine {
  Weight weight = Light;
};

struct VLine {
  Weight weight = Light;
};

// Up, Down, Left, Right
struct Cross : std::array<Weight, 4> {};

struct HDash {
  int num;
  Weight weight = Light;
};

struct VDash {
  int num;
  Weight weight = Light;
};

// Up, Down, Left, Right
struct DoubleCross : std::array<Weight, 4> {};

struct Arc {
  QuadDir dir;
};

struct Diagonal {
  bool forward = false;
  bool back = false;
};

struct HalfLine {
  Weight up = None;
  Weight down = None;
  Weight left = None;
  Weight right = None;
};

struct UpperBlock {
  float size;
};
struct LowerBlock {
  float size;
};
struct LeftBlock {
  float size;
};
struct RightBlock {
  float size;
};

enum ShadeType { SLight, SMedium, SDark };
struct Shade {
  ShadeType type;
};

struct Quadrant : std::set<QuadDir> {
  Quadrant(std::initializer_list<QuadDir> dirs)
    : std::set<QuadDir>(dirs) {}
};

struct Braille {
  char32_t charcode;
};

struct Underline {
  UnderlineType underlineType;
};

using DrawDesc = std::variant<
  HLine,
  VLine,
  Cross,
  HDash,
  VDash,
  DoubleCross,
  Arc,
  Diagonal,
  HalfLine,
  UpperBlock,
  LowerBlock,
  LeftBlock,
  RightBlock,
  Shade,
  Quadrant,
  Braille,
  Underline>;

struct Pen {
private:
  BLImage img;
  BLContext ctx;

  float dpiScale;

  float xsize;
  float ysize;

  float xhalf;
  float yhalf;

  float lightWidth;
  float heavyWidth;

  float underlineThickness;

  // internal data after drawing
  // Draw() returns a view on blData.pixelData
  BLImageData blData;

  // box drawing stuff
  float ToWidth(Weight weight);
  void DrawHLine(float start, float end, Weight weight);
  void DrawVLine(float start, float end, Weight weight);

  void DrawCross(const Cross& desc);
  void DrawHDash(const HDash& desc);
  void DrawVDash(const VDash& desc);
  void DrawDoubleCross(const DoubleCross& desc);
  void DrawArc(const Arc& desc);
  void DrawDiagonal(const Diagonal& desc);
  void DrawHalfLine(const HalfLine& desc);
  void DrawShade(const Shade& desc);
  void DrawQuadrant(const Quadrant& desc);

  void DrawBraille(const Braille& desc);

  void DrawUnderline(const Underline& desc);

public:
  using BufType = std::mdspan<uint32_t, std::dextents<size_t, 2>, std::layout_stride>;
  struct ImageData {
    BufType data;
    Region localPoss;
  };

  Pen() = default;
  Pen(glm::vec2 charSize, float underlineThickness, float dpiScale);
  ImageData Draw(const DrawDesc& desc);
};

}
