#pragma once

#include "app/size.hpp"
#include "editor/window.hpp"
#include "utils/region.hpp"
#include "gfx/render_texture.hpp"
#include "webgpu/webgpu_cpp.h"
#include "nvim/events/ui.hpp"

enum class CursorShape {
  Block,
  Horizontal,
  Vertical,
  None,
};

struct CursorMode {
  CursorShape cursorShape = CursorShape::None;

  int cellPercentage = 0;
  int blinkwait = 0;
  int blinkon = 0;
  int blinkoff = 0;
  int attrId = 0;

  std::string ToString() {
    std::string str = "ModeInfo: ";
    str += "cursorShape: " + std::to_string(int(cursorShape)) + ", ";
    str += "cellPercentage: " + std::to_string(cellPercentage) + ", ";
    str += "blinkwait: " + std::to_string(blinkwait) + ", ";
    str += "blinkon: " + std::to_string(blinkon) + ", ";
    str += "blinkoff: " + std::to_string(blinkoff) + ", ";
    str += "attrId: " + std::to_string(attrId) + ", ";
    return str;
  }
};

enum class BlinkState { Wait, On, Off };

struct Cursor {
  glm::vec2 size;

  bool dirty;
  int grid;
  int row;
  int col;

  glm::vec2 prevScrollOffset;

  glm::vec2 destPos;
  glm::vec2 maskPos; // cursor mask, actual position of cell

  CursorMode* cursorMode;

  Region startCorners;
  Region destCorners;
  Region corners;
  float cornerTime = 0.06; // shape transition time
  float cornerElasped;

  bool blink;
  float blinkElasped; // in milliseconds
  BlinkState blinkState;

  RenderTexture maskRenderTexture;
  wgpu::Buffer maskPosBuffer;
  wgpu::BindGroup maskPosBG;

  void Resize(glm::vec2 size, float dpi);
  void Goto(const event::GridCursorGoto& e);
  void SetMode(CursorMode* cursorMode);

  // returns true if destPos changes
  bool SetDestPos(const Win* currWin, const SizeHandler& sizes);
  void Update(float dt);

  bool ShouldRender();
};
