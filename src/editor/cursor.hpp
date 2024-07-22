#pragma once

#include "gfx/render_texture.hpp"
#include "webgpu/webgpu_cpp.h"
#include "glm/ext/vector_float2.hpp"
#include "nvim/events/parse.hpp"
#include "utils/region.hpp"
#include <string>

enum class CursorShape {
  Block,
  Horizontal,
  Vertical,
  None,
};

struct ModeInfo {
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

  glm::vec2 startPos;
  glm::vec2 destPos;
  glm::vec2 pos;
  float jumpTime = 0.06;
  float jumpElasped;

  ModeInfo* modeInfo;

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

  void Init(glm::vec2 size, float dpi);
  void Goto(const GridCursorGoto& e);
  bool SetDestPos(glm::vec2 destPos);
  void SetMode(ModeInfo* modeInfo);
  void Update(float dt);
  bool ShouldRender();
};
