#pragma once
#include <mdspan>
#include <variant>
#include "blend2d.h"

namespace box {

using BufType = std::mdspan<uint8_t, std::dextents<size_t, 2>>;

enum Weight : uint8_t { None, Light, Heavy, Double };
enum Side : uint8_t { Up, Down, Left, Right };

struct HLine {
  Weight weight = Light;
};

struct VLine {
  Weight weight = Light;
};

struct Cross : std::array<Weight, 4> {};

struct HDash {
  int num;
  Weight weight = Light;
};

struct VDash {
  int num;
  Weight weight = Light;
};

struct DoubleCross : std::array<Weight, 4> {};

enum ArcDir : uint8_t { UpLeft, UpRight, DownLeft, DownRight };
struct Arc {
  ArcDir dir;
};

struct HalfLine {
  Weight up = None;
  Weight down = None;
  Weight left = None;
  Weight right = None;
};

struct UpperBlock {
  double size;
};

struct LowerBlock {
  double size;
};

struct LeftBlock {
  double size;
};

struct RightBlock {
  double size;
};

struct Quadrant {
  bool upperLeft = false;
  bool upperRight = false;
  bool lowerLeft = false;
  bool lowerRight = false;
};

using DrawDesc = std::variant<
  HLine,
  VLine,
  Cross,
  HDash,
  VDash,
  DoubleCross,
  Arc,
  HalfLine,
  UpperBlock,
  LowerBlock,
  LeftBlock,
  RightBlock,
  Quadrant>;

struct Pen {
  BLImage img;
  BLContext ctx;

  // BufType canvas;
  double xsize;
  double ysize;
  double xhalf;
  double yhalf;

  double lightWidth;
  double heavyWidth;

  void Begin(double width, double height);
  BLImageData End();
  double ToWidth(Weight weight);

  void DrawRect(double left, double top, double width, double height);

  void DrawHLine(double start, double end, Weight weight);
  void DrawVLine(double start, double end, Weight weight);
  void DrawCross(const Cross& desc);
  void DrawHDash(const HDash& desc);
  void DrawVDash(const VDash& desc);
  void DrawDoubleCross(const DoubleCross& desc);
  void DrawArc(const Arc& desc);
  void DrawHalfLine(const HalfLine& desc);
  void DrawQuadrant(const Quadrant& desc);

  void Draw(const DrawDesc& desc);
};

}
