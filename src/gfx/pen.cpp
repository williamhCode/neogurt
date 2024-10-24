#include "pen.hpp"
#include "utils/logger.hpp"
#include "utils/variant.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace box {

void Pen::Begin(double width, double height) {
  xsize = width;
  ysize = height;

  xhalf = xsize / 2;
  yhalf = ysize / 2;

  lightWidth = xsize * 0.11;
  heavyWidth = xsize * 0.22;

  img.create(width, height, BL_FORMAT_PRGB32);
  ctx.begin(img);
  ctx.setFillStyle(BLRgba(1, 1, 1, 1));
  ctx.setCompOp(BL_COMP_OP_PLUS);
  ctx.clearAll();
}

BLImageData Pen::End() {
  ctx.end();
  BLImageData data;
  img.getData(&data);
  return data;
}

double Pen::ToWidth(Weight weight) {
  switch (weight) {
    case Light: return lightWidth;
    case Heavy: return heavyWidth;
    case Double: return lightWidth * 3;
    default: return 0;
  }
}

void Pen::DrawRect(double left, double top, double width, double height) {
  ctx.fillRect(left, top, width, height);
}

void Pen::DrawHLine(double start, double end, Weight weight) {
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

void Pen::DrawVLine(double start, double end, Weight weight) {
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
  double width = ToWidth(std::max(desc[Up], desc[Down]));
  double height = ToWidth(std::max(desc[Left], desc[Right]));
  double halfWidth = width / 2;
  double halfHeight = height / 2;

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

  DrawRect(xhalf - halfWidth, yhalf - halfHeight, width, height);
}

void Pen::DrawHDash(const HDash& desc) {
  double numDashes = desc.num;
  for (int x = 0; x < desc.num; x++) {
    double start = (x + 0.15) / numDashes * xsize;
    double end = (x + 0.85) / numDashes * xsize;
    DrawHLine(start, end, desc.weight);
  }
}

void Pen::DrawVDash(const VDash& desc) {
  double numDashes = desc.num;
  for (int y = 0; y < desc.num; y++) {
    double start = (y + 0.15) / numDashes * ysize;
    double end = (y + 0.85) / numDashes * ysize;
    DrawVLine(start, end, desc.weight);
  }
}

void Pen::DrawDoubleCross(const DoubleCross& desc) {
  // dimensions of center cross section
  double width = ToWidth(std::max(desc[Up], desc[Down]));
  double height = ToWidth(std::max(desc[Left], desc[Right]));
  double halfWidth = width / 2;
  double halfHeight = height / 2;

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
  for (int i = 0; i < 4; i++) {
    auto weight = desc[i];
    if (weight != None) numSides++;
    FillSide(weight, Side(i));
  }

  // fill up the cross sections
  double gridLeft = xhalf - (1.5 * lightWidth);
  double gridTop = yhalf - (1.5 * lightWidth);
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      if (grid[row][col] >= numSides) {
        double left = gridLeft + (col * lightWidth);
        double top = gridTop + (row * lightWidth);
        DrawRect(left, top, lightWidth, lightWidth);
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
  ctx.setTransform(transform);

  ctx.setStrokeWidth(lightWidth);
  if (xsize < ysize) {
    double radius = xhalf;
    double offset = yhalf - radius;
    ctx.strokeLine(xhalf, 0, xhalf, offset);
    ctx.strokeArc(0, offset, radius, radius, 0, M_PI_2);
  } else {
    double radius = yhalf;
    double offset = xhalf - radius;
    ctx.strokeLine(0, yhalf, offset, yhalf);
    ctx.strokeArc(offset, 0, radius, radius, 0, M_PI_2);
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

void Pen::DrawQuadrant(const Quadrant& desc) {
  if (desc.upperLeft) {
    DrawRect(0, 0, xhalf, yhalf);
  }
  if (desc.upperRight) {
    DrawRect(xhalf, 0, xhalf, yhalf);
  }
  if (desc.lowerLeft) {
    DrawRect(0, yhalf, xhalf, yhalf);
  }
  if (desc.lowerRight) {
    DrawRect(xhalf, yhalf, xhalf, yhalf);
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
    [this](const Arc& desc) { DrawArc(desc); },
    [this](const HalfLine& desc) { DrawHalfLine(desc); },
    [this](const UpperBlock& desc) { DrawRect(0, 0, xsize, desc.size * ysize); },
    [this](const LowerBlock& desc) { DrawRect(0, ysize - (desc.size * ysize), xsize, desc.size * ysize); },
    [this](const LeftBlock& desc) { DrawRect(0, 0, desc.size * xsize, ysize); },
    [this](const RightBlock& desc) { DrawRect(xsize - (desc.size * xsize), 0, desc.size * xsize, ysize); },
    [this](const Quadrant& desc) { DrawQuadrant(desc); }
  }, desc);
}

}
