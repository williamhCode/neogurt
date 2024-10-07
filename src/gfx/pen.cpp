#include "pen.hpp"
#include "utils/variant.hpp"
#include <algorithm>
#include <cassert>

static float fpart(float x) {
  return x - std::floor(x);
}

static float rfpart(float x) {
  return 1.0f - fpart(x);
}

namespace box {

void Pen::SetData(BufType& _data, float dpiScale) {
  data = _data;
  xsize = data.extent(1);
  ysize = data.extent(0);
  xcenter = xsize / 2;
  ycenter = ysize / 2;

  lightWidth = std::fmax(xsize * 0.11, 1);
  heavyWidth = std::fmax(xsize * 0.22, 1);
}

float Pen::ToWidth(Weight weight) {
  if (weight == None) {
    return 0;
  }
  if (weight == Light) {
    return lightWidth;
  }
  return heavyWidth;
}

void Pen::Fill(int x, int y, uint8_t alpha) {
  // use integer promotion
  data[y, x] = std::min(data[y, x] + alpha, 255);
}

void Pen::DrawRect(float left, float top, float width, float height) {
  assert(left >= 0 && top >= 0);
  assert(width > 0 && height > 0);

  float right = left + width;
  float bottom = top + height;
  assert(right <= xsize && bottom <= ysize);

  for (int y = top; y < bottom; y++) {
    for (int x = left; x < right; x++) {
      float alpha = 1;
      if (x < left) {
        alpha *= rfpart(left);
      } else if (x > right - 1) {
        alpha *= fpart(right);
      }
      if (y < top) {
        alpha *= rfpart(top);
      } else if (y > bottom - 1) {
        alpha *= fpart(bottom);
      }
      Fill(x, y, alpha * 255);
    }
  }
}

void Pen::DrawHLine(float start, float end, Weight weight) {
  assert(weight != None);
  float lineWidth = ToWidth(weight);
  float halfWidth = lineWidth / 2;

  float left = start;
  float top = ysize / 2 - halfWidth;
  DrawRect(left, top, end - start, lineWidth);
}

void Pen::DrawVLine(float start, float end, Weight weight) {
  assert(weight != None);
  float lineWidth = ToWidth(weight);
  float halfWidth = lineWidth / 2;

  float left = xsize / 2 - halfWidth;
  float top = start;
  DrawRect(left, top, lineWidth, end - start);
}

void Pen::DrawCross(const Cross& desc) {
  // dimensions of center, draw this separate
  float width = std::max(ToWidth(desc.top), ToWidth(desc.bottom));
  float height = std::max(ToWidth(desc.left), ToWidth(desc.right));
  float halfWidth = width / 2;
  float halfHeight = height / 2;

  if (desc.top != None) {
    DrawVLine(0, ycenter - halfHeight, desc.top);
  }
  if (desc.bottom != None) {
    DrawVLine(ycenter + halfHeight, ysize, desc.bottom);
  }
  if (desc.left != None) {
    DrawHLine(0, xcenter - halfWidth, desc.left);
  }
  if (desc.right != None) {
    DrawHLine(xcenter + halfWidth, xsize, desc.right);
  }

  DrawRect(xcenter - halfWidth, ycenter - halfHeight, width, height);
}

void Pen::DrawHDash(const HDash& desc) {
  float numDashes = desc.num;
  for (int x = 0; x < desc.num; x++) {
    float start = x / numDashes * xsize;
    float end = (x + 0.7) / numDashes * xsize;
    DrawHLine(start, end, desc.weight);
  }
}

void Pen::DrawVDash(const VDash& desc) {
  float numDashes = desc.num;
  for (int y = 0; y < desc.num; y++) {
    float start = y / numDashes * ysize;
    float end = (y + 0.7) / numDashes * ysize;
    DrawVLine(start, end, desc.weight);
  }
}

void Pen::Draw(const DrawDesc& desc) {
  std::visit(overloaded{
    [this](const HLine& desc) { DrawHLine(0, xsize, desc.weight); },
    [this](const VLine& desc) { DrawVLine(0, ysize, desc.weight); },
    [this](const Cross& desc) { DrawCross(desc); },
    [this](const HDash& desc) { DrawHDash(desc); },
    [this](const VDash& desc) { DrawVDash(desc); },
  }, desc);
}

}
