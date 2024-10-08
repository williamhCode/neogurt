#pragma once
#include <mdspan>
#include <variant>

namespace box {

using BufType = std::mdspan<uint8_t, std::dextents<size_t, 2>>;

enum Weight : uint8_t { None, Light, Heavy, Double };
enum Side : uint8_t { Top, Bottom, Left, Right };

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

struct HalfLine {
  Weight top = None;
  Weight bottom = None;
  Weight left = None;
  Weight right = None;
};

using DrawDesc = std::variant<HLine, VLine, Cross, HDash, VDash, DoubleCross, HalfLine>;

struct Pen {
  BufType data;
  float xsize;
  float ysize;
  float xcenter;
  float ycenter;

  float lightWidth;
  float heavyWidth;
  void SetData(BufType& data, float dpiScale);
  float ToWidth(Weight weight);

  void Fill(int x, int y, uint8_t alpha);
  void DrawRect(float left, float top, float width, float height);
  void DrawHLine(float ypos, float start, float end, float lineWidth);
  void DrawVLine(float xpos, float start, float end, float lineWidth);

  void DrawHLine(float start, float end, Weight weight);
  void DrawVLine(float start, float end, Weight weight);

  void DrawCross(const Cross& desc);

  void DrawHDash(const HDash& desc);
  void DrawVDash(const VDash& desc);

  void DrawDoubleCross(const DoubleCross& desc);

  void DrawHalfLine(const HalfLine& desc);

  void Draw(const DrawDesc& desc);
};

}
