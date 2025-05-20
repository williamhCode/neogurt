#include "./shape_pen.hpp"
#include "utils/logger.hpp"
#include "utils/mdspan.hpp"
#include "utils/variant.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace shape {

Pen::Pen(glm::vec2 charSize, float _underlineThickness, float _dpiScale)
    : dpiScale(_dpiScale) {
  xsize = charSize.x * dpiScale;
  ysize = charSize.y * dpiScale;

  xhalf = xsize / 2;
  yhalf = ysize / 2;

  lightWidth = xsize * 0.11;
  heavyWidth = xsize * 0.22;

  underlineThickness = _underlineThickness * dpiScale;
  // make sure underline at least one virtual pixel
  underlineThickness = std::max(underlineThickness, dpiScale);
}

float Pen::ToWidth(Weight weight) {
  switch (weight) {
    case Light: return lightWidth;
    case Heavy: return heavyWidth;
    case Double: return lightWidth * 3;
    default: return 0;
  }
}

void Pen::DrawHLine(float start, float end, Weight weight) {
  assert(weight != None);
  if (weight == Double) {
    ctx.setStrokeWidth(lightWidth);
    ctx.strokeLine(start, yhalf - lightWidth, end, yhalf - lightWidth);
    ctx.strokeLine(start, yhalf + lightWidth, end, yhalf + lightWidth);
  } else {
    ctx.setStrokeWidth(ToWidth(weight));
    ctx.strokeLine(start, yhalf, end, yhalf);
  }
}

void Pen::DrawVLine(float start, float end, Weight weight) {
  assert(weight != None);
  if (weight == Double) {
    ctx.setStrokeWidth(lightWidth);
    ctx.strokeLine(xhalf - lightWidth, start, xhalf - lightWidth, end);
    ctx.strokeLine(xhalf + lightWidth, start, xhalf + lightWidth, end);
  } else {
    ctx.setStrokeWidth(ToWidth(weight));
    ctx.strokeLine(xhalf, start, xhalf, end);
  }
}

void Pen::DrawCross(const Cross& desc) {
  // dimensions of center, draw this separate
  float width = ToWidth(std::max(desc[Up], desc[Down]));
  float height = ToWidth(std::max(desc[Left], desc[Right]));
  float halfWidth = width / 2;
  float halfHeight = height / 2;

  if (desc[Up] != None) {
    DrawVLine(0, yhalf - halfHeight, desc[Up]);
  }
  if (desc[Down] != None) {
    DrawVLine(yhalf + halfHeight, ysize, desc[Down]);
  }
  if (desc[Left] != None) {
    DrawHLine(0, xhalf - halfWidth, desc[Left]);
  }
  if (desc[Right] != None) {
    DrawHLine(xhalf + halfWidth, xsize, desc[Right]);
  }

  ctx.fillRect(xhalf - halfWidth, yhalf - halfHeight, width, height);
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
  float width = ToWidth(std::max(desc[Up], desc[Down]));
  float height = ToWidth(std::max(desc[Left], desc[Right]));
  float halfWidth = width / 2;
  float halfHeight = height / 2;

  // draw lines until center
  if (desc[Up] != None) {
    DrawVLine(0, yhalf - halfHeight, desc[Up]);
  }
  if (desc[Down] != None) {
    DrawVLine(yhalf + halfHeight, ysize, desc[Down]);
  }
  if (desc[Left] != None) {
    DrawHLine(0, xhalf - halfWidth, desc[Left]);
  }
  if (desc[Right] != None) {
    DrawHLine(xhalf + halfWidth, xsize, desc[Right]);
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

    If a grid cell adds up to greater or equal to the number of sides,
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
      if (side == Up || side == Down) {
        IncCol(1);
      } else if (side == Left || side == Right) {
        IncRow(1);
      }
      grid[1][1]++;
    } else if (weight == Double) {
      if (side == Up) {
        IncCol(0);
        IncCol(2);
        grid[2][1]++;
      } else if (side == Down) {
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
  for (size_t i = 0; i < 4; i++) {
    auto weight = desc[i];
    if (weight != None) numSides++;
    FillSide(weight, Side(i));
  }

  // fill up the cross sections
  float gridLeft = xhalf - (1.5 * lightWidth);
  float gridTop = yhalf - (1.5 * lightWidth);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      if (grid[row][col] >= numSides) {
        float left = gridLeft + (col * lightWidth);
        float top = gridTop + (row * lightWidth);
        ctx.fillRect(left, top, lightWidth, lightWidth);
      }
    }
  }
}

void Pen::DrawArc(const Arc& desc) {
  auto transform = BLMatrix2D::makeIdentity();
  switch (desc.dir) {
    case UpLeft:
      break;
    case UpRight:
      // flip horizontally
      transform.translate(xhalf, 0);
      transform.scale(-1, 1);
      transform.translate(-xhalf, 0);
      break;
    case DownLeft:
      // flip vertically
      transform.translate(0, yhalf);
      transform.scale(1, -1);
      transform.translate(0, -yhalf);
      break;
    case DownRight:
      // flip horizontally and vertically
      transform.rotate(M_PI, xhalf, yhalf);
      break;
  }
  ctx.applyTransform(transform);

  ctx.setStrokeWidth(lightWidth);
  if (xsize < ysize) {
    float radius = xhalf;
    float offset = yhalf - radius;
    ctx.strokeLine(xhalf, 0, xhalf, offset);
    ctx.strokeArc(0, offset, radius, radius, 0, M_PI_2);
  } else {
    float radius = yhalf;
    float offset = xhalf - radius;
    ctx.strokeLine(0, yhalf, offset, yhalf);
    ctx.strokeArc(offset, 0, radius, radius, 0, M_PI_2);
  }
}

void Pen::DrawDiagonal(const Diagonal& desc) {
  // make double diagonal has same effect as gpu blend
  // so cross section is more consistent
  ctx.setCompOp(BL_COMP_OP_SRC_OVER);

  float ratio = xsize / ysize;
  float a = lightWidth;
  float b = lightWidth * ratio;
  float horiSize = std::sqrt(a * a + b * b) / 2;
  if (desc.back) {
    BLPoint poly[4] = {
      {horiSize, 0},
      {xsize + horiSize, ysize},
      {xsize - horiSize, ysize},
      {-horiSize, 0},
    };
    ctx.fillPolygon(poly, 4);
  }
  if (desc.forward) {
    BLPoint poly[4] = {
      {xsize + horiSize, 0},
      {horiSize, ysize},
      {-horiSize, ysize},
      {xsize - horiSize, 0},
    };
    ctx.fillPolygon(poly, 4);
  }
}

void Pen::DrawHalfLine(const HalfLine& desc) {
  if (desc.up != None) {
    DrawVLine(0, yhalf, desc.up);
  }
  if (desc.down != None) {
    DrawVLine(yhalf, ysize, desc.down);
  }
  if (desc.left != None) {
    DrawHLine(0, xhalf, desc.left);
  }
  if (desc.right != None) {
    DrawHLine(xhalf, xsize, desc.right);
  }
}

void Pen::DrawShade(const Shade& desc) {
  int numHori = 5;
  int numVert = (ysize / xsize) * numHori;
  // round to nearest odd
  numVert = numVert % 2 == 0 ? numVert + 1 : numVert;

  float horiSize = xsize / numHori;
  float vertSize = ysize / numVert;
  float horiHalf = horiSize / 2;
  float vertHalf = vertSize / 2;

  switch (desc.type) {
    case SLight: {
      for (int y = 0; y < numVert; y++) {
        for (int x = 0; x < numHori; x++) {
          ctx.fillRect(x * horiSize, y * vertSize, horiHalf, vertHalf);
        }
      }
      break;
    }
    case SMedium: {
      for (int y = 0; y < numVert * 2; y++) {
        for (int x = 0; x < numHori; x++) {
          float xPos = y % 2 == 0 ? x : x + 0.5;
          ctx.fillRect(xPos * horiSize, y * vertHalf, horiHalf, vertHalf);
        }
      }
      break;
    }
    case SDark: {
      for (int y = 0; y < numVert * 2; y++) {
        for (int x = 0; x < numHori * 2; x++) {
          if (y % 2 == 1 && x % 2 == 1) continue;
          ctx.fillRect(x * horiHalf, y * vertHalf, horiHalf, vertHalf);
        }
      }
      break;
    }
  }
}

void Pen::DrawQuadrant(const Quadrant& desc) {
  if (desc.contains(UpLeft)) {
    ctx.fillRect(0, 0, xhalf, yhalf);
  }
  if (desc.contains(UpRight)) {
    ctx.fillRect(xhalf, 0, xhalf, yhalf);
  }
  if (desc.contains(DownLeft)) {
    ctx.fillRect(0, yhalf, xhalf, yhalf);
  }
  if (desc.contains(DownRight)) {
    ctx.fillRect(xhalf, yhalf, xhalf, yhalf);
  }
}

void Pen::DrawBraille(const Braille& desc) {
  // order, hex value
  // 0 3    1 8
  // 1 4    2 10
  // 2 5    4 20
  // 6 7    8 80
  uint32_t hexVal = desc.charcode - 0x2800;

  static constexpr std::array<glm::vec2, 8> brailleOffsets({
    {1/4., 1/8.},
    {1/4., 3/8.},
    {1/4., 5/8.},
    {3/4., 1/8.},
    {3/4., 3/8.},
    {3/4., 5/8.},
    {1/4., 7/8.},
    {3/4., 7/8.},
  });

  auto charSize = glm::vec2(xsize, ysize);
  float radius = std::min(charSize.x / 2, charSize.y / 4) / 2;
  radius *= 0.6; // padding

  for (size_t dotIndex = 0; dotIndex < brailleOffsets.size(); dotIndex++) {
    if (hexVal & (1 << dotIndex)) {
      auto centerPos = brailleOffsets[dotIndex] * charSize;
      ctx.fillCircle(centerPos.x, centerPos.y, radius);
    }
  }
}

void Pen::DrawUnderline(const Underline& desc) {
  // ctx.setCompOp(BL_COMP_OP_SRC_OVER);

  switch (desc.underlineType) {
    case UnderlineType::Underline: {
      ctx.fillRect(0, 0, xsize, underlineThickness);
      break;
    }

    case UnderlineType::Undercurl: {
      float height = underlineThickness * 3;
      // TODO: finish ts
      ctx.fillRect(0, 0, xsize, underlineThickness);
      break;
    }

    case UnderlineType::Underdouble: {
      ctx.fillRect(0, 0, xsize, underlineThickness);
      ctx.fillRect(0, underlineThickness * 2, xsize, underlineThickness);
      break;
    }

    case UnderlineType::Underdotted: {
      float width = xsize / 8;
      float height = underlineThickness * 1.5;
      ctx.fillRect(0, 0, width, height);
      ctx.fillRect(width * 2, 0, width, height);
      ctx.fillRect(width * 4, 0, width, height);
      ctx.fillRect(width * 6, 0, width, height);
      break;
    }

    case UnderlineType::Underdashed: {
      float width = xsize / 3;
      ctx.fillRect(0, 0, width, underlineThickness);
      ctx.fillRect(width * 2, 0, width, underlineThickness);
      break;
    }
  }
}

Pen::ImageData Pen::Draw(const DrawDesc& desc) {
  // init ---------------------------
  int xoffset = 0;
  int yoffset = 0;

  // actual size of data
  int dataWidth = xsize;
  int dataHeight = ysize;

  // diagonal needs extra space
  if (std::holds_alternative<Diagonal>(desc)) {
    xoffset += xhalf / 2;
    dataWidth += xhalf;
  }

  // begin -----------------------------
  img.create(dataWidth, dataHeight, BL_FORMAT_PRGB32);
  ctx.begin(img);
  ctx.clearAll();

  ctx.setFillStyle(BLRgba(1, 1, 1, 1));
  ctx.setCompOp(BL_COMP_OP_PLUS);
  ctx.translate(xoffset, yoffset);

  // draw ------------------------------
  std::visit(overloaded{
    [this](const HLine& desc) { DrawHLine(0, xsize, desc.weight); },
    [this](const VLine& desc) { DrawVLine(0, ysize, desc.weight); },
    [this](const Cross& desc) { DrawCross(desc); },
    [this](const HDash& desc) { DrawHDash(desc); },
    [this](const VDash& desc) { DrawVDash(desc); },
    [this](const DoubleCross& desc) { DrawDoubleCross(desc); },
    [this](const Arc& desc) { DrawArc(desc); },
    [this](const Diagonal& desc) { DrawDiagonal(desc); },
    [this](const HalfLine& desc) { DrawHalfLine(desc); },
    [this](const Shade& desc) { DrawShade(desc); },
    [this](const UpperBlock& desc) { ctx.fillRect(0, 0, xsize, desc.size * ysize); },
    [this](const LowerBlock& desc) { ctx.fillRect(0, ysize - (desc.size * ysize), xsize, desc.size * ysize); },
    [this](const LeftBlock& desc) { ctx.fillRect(0, 0, desc.size * xsize, ysize); },
    [this](const RightBlock& desc) { ctx.fillRect(xsize - (desc.size * xsize), 0, desc.size * xsize, ysize); },
    [this](const Quadrant& desc) { DrawQuadrant(desc); },
    [this](const Braille& desc) { DrawBraille(desc); },
    [this](const Underline& desc) { DrawUnderline(desc); },
  }, desc);

  // end -------------------------------
  ctx.end();
  img.getData(&blData);

  // create data ----------------------------
  std::extents shape{dataHeight, dataWidth};
  std::array strides{blData.stride / sizeof(uint32_t), 1uz};
  auto data = std::mdspan(
    static_cast<uint32_t*>(blData.pixelData),
    std::layout_stride::mapping{shape, strides}
  );

  // memory optimization
  // find the bounds of data and create span within the bounds
  // basically discards empty cells
  size_t xmin = data.extent(1);
  size_t ymin = data.extent(0);
  size_t xmax = 0;
  size_t ymax = 0;

  for (size_t y = 0; y < data.extent(0); y++) {
    for (size_t x = 0; x < data.extent(1); x++) {
      if (data[y, x] != 0) {
        xmin = std::min(xmin, x);
        ymin = std::min(ymin, y);
        xmax = std::max(xmax, x);
        ymax = std::max(ymax, y);
      }
    }
  }

  // check empty image
  if (xmin > xmax || ymin > ymax) {
    return {};
  }

  size_t subWidth = xmax - xmin + 1;
  size_t subHeight = ymax - ymin + 1;

  // NOTE: use submdspan for c++26
  auto subData = SubMdspan2d(data, {ymin, xmin}, {subHeight, subWidth});

  return {
    subData,
    MakeRegion(
      glm::vec2((int)xmin - xoffset, (int)ymin - yoffset) / dpiScale,
      glm::vec2(subWidth, subHeight) / dpiScale
    ),
  };
}

} // namespace shape
