#pragma once
#include <mdspan>
#include <variant>

namespace box {

using BufType = std::mdspan<uint8_t, std::dextents<size_t, 2>>;

enum Weight {
  None,
  Light,
  Heavy,
};

struct HLine {
  Weight weight = Light;
};

struct VLine {
  Weight weight = Light;
};

struct Cross {
  Weight top = None;
  Weight bottom = None;
  Weight left = None;
  Weight right = None;
};

struct HDash {
  int num;
  Weight weight = Light;
};

struct VDash {
  int num;
  Weight weight = Light;
};

using DrawDesc = std::variant<HLine, VLine, Cross, HDash, VDash>;

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

  void DrawHLine(float start, float end, Weight weight);
  void DrawVLine(float start, float end, Weight weight);

  void DrawCross(const Cross& desc);

  void DrawHDash(const HDash& desc);
  void DrawVDash(const VDash& desc);

  void Draw(const DrawDesc& desc);
};

}
