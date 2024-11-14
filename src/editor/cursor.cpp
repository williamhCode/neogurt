#include "cursor.hpp"

#include "webgpu_tools/utils/webgpu.hpp"
#include "gfx/instance.hpp"
#include "glm/common.hpp"
#include "utils/easing_funcs.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"

using namespace wgpu;

void Cursor::Resize(glm::vec2 _size, float dpi) {
  size = _size;

  maskRenderTexture = RenderTexture(size, dpi, TextureFormat::R8Unorm);

  maskPosBuffer = utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &maskPos);

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

bool Cursor::SetDestPos(const Win* currWin, const SizeHandler& sizes) {
  if (currWin == nullptr) return false;

  const auto& winTex = currWin->sRenderTexture;
  auto scrollOffset = winTex.scrolling
                        ? glm::vec2(0, (winTex.scrollDist - winTex.scrollCurr))
                        : glm::vec2(0);

  if (blink && (dirty || scrollOffset != prevScrollOffset)) {
    // reset blink state if
    // - move to new window or row/col
    // - scrolling
    blinkState = BlinkState::Wait;
    blinkElasped = 0.0;
    prevScrollOffset = scrollOffset;
  }

  // set relative to window top-left
  auto cursorPos = glm::vec2(col, row) * sizes.charSize + scrollOffset;
  auto newMaskPos = cursorPos;

  // clamp to window margins
  auto minPos = glm::vec2(0, currWin->margins.top) * sizes.charSize;
  auto maxPos =
    glm::vec2{
      currWin->grid.width,
      currWin->grid.height - currWin->margins.bottom - 1,
    } *
    sizes.charSize;
  cursorPos = glm::max(cursorPos, minPos);
  cursorPos = glm::min(cursorPos, maxPos);

  auto winOffset = glm::vec2(currWin->startCol, currWin->startRow) * sizes.charSize;

  // set to global position
  cursorPos += winOffset + sizes.offset;
  newMaskPos += winOffset + sizes.offset;

  if (newMaskPos != maskPos) {
    maskPos = newMaskPos;
    ctx.queue.WriteBuffer(maskPosBuffer, 0, &maskPos, sizeof(glm::vec2));
  }

  if (cursorPos == destPos) return false;
  destPos = cursorPos;

  startPos = pos;
  jumpElasped = 0.0;

  return true;
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
