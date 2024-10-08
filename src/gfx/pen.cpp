#include "pen.hpp"
#include "utils/logger.hpp"
#include "utils/variant.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace box {

static float fpart(float x) {
  return x - std::floor(x);
}

static float rfpart(float x) {
  return 1.0f - fpart(x);
}

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
  switch (weight) {
    case Light: return lightWidth;
    case Heavy: return heavyWidth;
    case Double: return lightWidth * 3;
    default: return 0;
  }
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

void Pen::DrawHLine(float ypos, float start, float end, float lineWidth) {
  assert(start <= end);
  assert(lineWidth > 0);
  float halfWidth = lineWidth / 2;
  float left = start;
  float top = ypos - halfWidth;
  DrawRect(left, top, end - start, lineWidth);
}

void Pen::DrawVLine(float xpos, float start, float end, float lineWidth) {
  assert(start <= end);
  assert(lineWidth > 0);
  float halfWidth = lineWidth / 2;
  float left = xpos - halfWidth;
  float top = start;
  DrawRect(left, top, lineWidth, end - start);
}

void Pen::DrawHLine(float start, float end, Weight weight) {
  assert(weight != None);
  if (weight == Double) {
    float lineWidth = lightWidth;
    DrawHLine(ycenter - lineWidth, start, end, lineWidth);
    DrawHLine(ycenter + lineWidth, start, end, lineWidth);
  } else {
    DrawHLine(ycenter, start, end, ToWidth(weight));
  }
}

void Pen::DrawVLine(float start, float end, Weight weight) {
  assert(weight != None);
  if (weight == Double) {
    float lineWidth = lightWidth;
    DrawVLine(xcenter - lineWidth, start, end, lineWidth);
    DrawVLine(xcenter + lineWidth, start, end, lineWidth);
  } else {
    DrawVLine(xcenter, start, end, ToWidth(weight));
  }
}

void Pen::DrawCross(const Cross& desc) {
  // dimensions of center, draw this separate
  float width = ToWidth(std::max(desc[Top], desc[Bottom]));
  float height = ToWidth(std::max(desc[Left], desc[Right]));
  float halfWidth = width / 2;
  float halfHeight = height / 2;

  if (desc[Top] != None) {
    DrawVLine(0, ycenter - halfHeight, desc[Top]);
  }
  if (desc[Bottom] != None) {
    DrawVLine(ycenter + halfHeight, ysize, desc[Bottom]);
  }
  if (desc[Left] != None) {
    DrawHLine(0, xcenter - halfWidth, desc[Left]);
  }
  if (desc[Right] != None) {
    DrawHLine(xcenter + halfWidth, xsize, desc[Right]);
  }

  DrawRect(xcenter - halfWidth, ycenter - halfHeight, width, height);
}

void Pen::DrawHDash(const HDash& desc) {
  float numDashes = desc.num;
  for (int x = 0; x < desc.num; x++) {
    float start = (x + 0.15) / numDashes * xsize;
    float end = (x + 0.85) / numDashes * xsize;
    DrawHLine(start, end, desc.weight);
  }
}

void Pen::DrawVDash(const VDash& desc) {
  float numDashes = desc.num;
  for (int y = 0; y < desc.num; y++) {
    float start = (y + 0.15) / numDashes * ysize;
    float end = (y + 0.85) / numDashes * ysize;
    DrawVLine(start, end, desc.weight);
  }
}

void Pen::DrawDoubleCross(const DoubleCross& desc) {
  // dimensions of center cross section
  float width = ToWidth(std::max(desc[Top], desc[Bottom]));
  float height = ToWidth(std::max(desc[Left], desc[Right]));
  float halfWidth = width / 2;
  float halfHeight = height / 2;

  // draw lines until center
  if (desc[Top] != None) {
    DrawVLine(0, ycenter - halfHeight, desc[Top]);
  }
  if (desc[Bottom] != None) {
    DrawVLine(ycenter + halfHeight, ysize, desc[Bottom]);
  }
  if (desc[Left] != None) {
    DrawHLine(0, xcenter - halfWidth, desc[Left]);
  }
  if (desc[Right] != None) {
    DrawHLine(xcenter + halfWidth, xsize, desc[Right]);
  }
  
  /*
  Do some weird cross section shenanigans.
  I spent way too much time coming up with this,
  but hey it works kinda neat.

  Light
  Top      Bottom   Left     Right
  0 1 0    0 1 0    0 0 0    0 0 0
  0 2 0    0 2 0    1 2 1    1 2 1
  0 1 0    0 1 0    0 0 0    0 0 0

  Double
  Top      Bottom   Left     Right
  1 0 1    1 1 1    1 1 1    1 1 1
  1 0 1    1 0 1    0 0 1    1 0 0
  1 1 1    1 0 1    1 1 1    1 1 1

  If a grid adds up to greater or equal to the number of sides,
  draw a square there.
  */

  int grid[3][3] = {};
  auto IncCol = [&](int col) {
    for (int row = 0; row < 3; row++) {
      grid[row][col]++;
    }
  };
  auto IncRow = [&](int row) {
    for (int col = 0; col < 3; col++) {
      grid[row][col]++;
    }
  };

  auto FillSide = [&] (Weight weight, Side side) {
    if (weight == None) return;
    if (weight == Light) {
      if (side == Top || side == Bottom) {
        IncCol(1);
      } else if (side == Left || side == Right) {
        IncRow(1);
      }
      grid[1][1]++;
    } else if (weight == Double) {
      if (side == Top) {
        IncCol(0);
        IncCol(2);
        grid[2][1]++;
      } else if (side == Bottom) {
        IncCol(0);
        IncCol(2);
        grid[0][1]++;
      } else if (side == Left) {
        IncRow(0);
        IncRow(2);
        grid[1][2]++;
      } else if (side == Right) {
        IncRow(0);
        IncRow(2);
        grid[1][0]++;
      }
    }
  };

  int numSides = 0;
  for (int i = 0; i < 4; i++) {
    auto weight = desc[i];
    if (weight != None) numSides++;
    FillSide(weight, Side(i));
  }

  // fill up the cross sections
  float gridLeft = xcenter - (1.5 * lightWidth);
  float gridTop = ycenter - (1.5 * lightWidth);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      if (grid[row][col] >= numSides) {
        float left = gridLeft + (col * lightWidth);
        float top = gridTop + (row * lightWidth);
        DrawRect(left, top, lightWidth, lightWidth);
      }
    }
  }
}

void Pen::DrawHalfLine(const HalfLine& desc) {
  if (desc.top != None) {
    DrawVLine(0, ycenter, desc.top);
  }
  if (desc.bottom != None) {
    DrawVLine(ycenter, ysize, desc.bottom);
  }
  if (desc.left != None) {
    DrawHLine(0, xcenter, desc.left);
  }
  if (desc.right != None) {
    DrawHLine(xcenter, xsize, desc.right);
  }
}

void Pen::Draw(const DrawDesc& desc) {
  std::visit(overloaded{
    [this](const HLine& desc) { DrawHLine(0, xsize, desc.weight); },
    [this](const VLine& desc) { DrawVLine(0, ysize, desc.weight); },
    [this](const Cross& desc) { DrawCross(desc); },
    [this](const HDash& desc) { DrawHDash(desc); },
    [this](const VDash& desc) { DrawVDash(desc); },
    [this](const DoubleCross& desc) { DrawDoubleCross(desc); },
    [this](const HalfLine& desc) { DrawHalfLine(desc); },
  }, desc);
}

}
