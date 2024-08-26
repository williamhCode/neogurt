#include "cursor.hpp"

#include "webgpu_tools/utils/webgpu.hpp"
#include "gfx/instance.hpp"
#include "glm/common.hpp"
#include "utils/easing_funcs.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"

using namespace wgpu;

void Cursor::Init(glm::vec2 _size, float dpi) {
  size = _size;

  maskRenderTexture = RenderTexture(size, dpi, TextureFormat::R8Unorm);

  maskPosBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2));

  maskPosBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.cursorMaskPosBGL,
    {
      {0, maskPosBuffer},
    }
  );

  dirty = true;
}

void Cursor::Goto(const event::GridCursorGoto& e) {
  dirty = true;
  grid = e.grid;
  row = e.row;
  col = e.col;
}

bool Cursor::SetDestPos(glm::vec2 _destPos) {
  if (_destPos == destPos) return false;

  destPos = _destPos;
  startPos = pos;
  jumpElasped = 0.0;

  if (blink) {
    blinkState = BlinkState::Wait;
    blinkElasped = 0.0;
  }

  ctx.queue.WriteBuffer(maskPosBuffer, 0, &destPos, sizeof(glm::vec2));

  return true;
}

void Cursor::SetMode(CursorMode* _modeInfo) {
  cursorMode = _modeInfo;

  float ratio = cursorMode->cellPercentage / 100.0;
  auto currSize = this->size;
  glm::vec2 offset(0, 0);
  switch (cursorMode->cursorShape) {
    case CursorShape::Block: break;
    case CursorShape::Horizontal:
      currSize.y *= ratio;
      offset.y = this->size.y * (1 - ratio);
      break;
    case CursorShape::Vertical: currSize.x *= ratio; break;
    case CursorShape::None:
      LOG_ERR("Invalid cursor shape");
      break;
  }

  destCorners = MakeRegion(offset, currSize);
  startCorners = corners;
  cornerElasped = 0.0;

  if (blink) {
    blinkState = BlinkState::Wait;
    blinkElasped = 0.0;
  }
  blink = cursorMode->blinkwait != 0 && cursorMode->blinkon != 0 && cursorMode->blinkoff != 0;
}

void Cursor::Update(float dt) {
  // position
  if (pos != destPos) {
    jumpElasped += dt;
    if (jumpElasped >= jumpTime) {
      pos = destPos;
      jumpElasped = 0.0;
    } else {
      // use smoothstep
      float t = jumpElasped / jumpTime;
      float y = EaseOutCubic(t);
      pos = glm::mix(startPos, destPos, y);
    }
  }

  // Shape transition
  if (corners != destCorners) {
    cornerElasped += dt;
    if (cornerElasped >= cornerTime) {
      corners = destCorners;
      cornerElasped = 0.0;
    } else {
      float t = cornerElasped / cornerTime;
      for (size_t i = 0; i < 4; i++) {
        float y = EaseOutQuad(t);
        corners[i] = glm::mix(startCorners[i], destCorners[i], y);
      }
    }
  }

  // blink
  if (blink) {
    blinkElasped += dt * 1000; // in milliseconds
    switch (blinkState) {
      case BlinkState::Wait:
        if (blinkElasped >= cursorMode->blinkwait) {
          blinkState = BlinkState::On;
          blinkElasped = 0.0;
        }
        break;
      case BlinkState::On:
        if (blinkElasped >= cursorMode->blinkon) {
          blinkState = BlinkState::Off;
          blinkElasped = 0.0;
        }
        break;
      case BlinkState::Off:
        if (blinkElasped >= cursorMode->blinkoff) {
          blinkState = BlinkState::On;
          blinkElasped = 0.0;
        }
        break;
    }
  }
}

bool Cursor::ShouldRender() {
  return cursorMode != nullptr && cursorMode->cursorShape != CursorShape::None &&
         blinkState != BlinkState::Off;
}
