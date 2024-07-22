#include "render_texture.hpp"

#include "utils/logger.hpp"
#include "webgpu_tools/utils/webgpu.hpp"
#include "gfx/instance.hpp"
#include "glm/common.hpp"
#include "utils/line.hpp"
#include "utils/easing_funcs.hpp"
#include <memory>

using namespace wgpu;

RenderTexture::RenderTexture(
  glm::vec2 _size, float dpiScale, wgpu::TextureFormat format, const void* data
)
    : size(_size) {
  camera = Ortho2D(size);

  auto fbSize = size * dpiScale;
  texture = utils::CreateRenderTexture(ctx.device, fbSize, format, data);
  textureView = texture.CreateView();

  auto textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
  }));

  textureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureBGL,
    {
      {0, textureView},
      {1, textureSampler},
    }
  );

  renderData.CreateBuffers(1);
}

void RenderTexture::UpdatePos(glm::vec2 pos, std::optional<Rect> region) {
  renderData.ResetCounts();

  Region positions;
  Region uvs;

  if (region == std::nullopt) {
    positions = MakeRegion(pos, size);
    uvs = MakeRegion({0, 0}, {1, 1});
  } else {
    positions = MakeRegion(pos, region->size);
    region->pos /= size, region->size /= size;
    uvs = region->Region();
  }

  auto& quad = renderData.NextQuad();
  for (size_t i = 0; i < 4; i++) {
    quad[i].position = positions[i];
    quad[i].uv = uvs[i];
  }
  renderData.WriteBuffers();
}

void RenderTexture::UpdateCameraPos(glm::vec2 pos) {
  camera.Resize(size, pos);
}

// ------------------------------------------------------------------
ScrollableRenderTexture::ScrollableRenderTexture(
  glm::vec2 _size, float _dpiScale, glm::vec2 _charSize
)
    : posOffset(0), size(_size), dpiScale(_dpiScale), charSize(_charSize) {

  textureHeight = size.y / maxNumTexPerPage;
  rowsPerTexture = glm::ceil(textureHeight / charSize.y);
  rowsPerTexture = glm::max(rowsPerTexture, 1);
  textureHeight = rowsPerTexture * charSize.y;

  int numTexPerPage = glm::ceil(size.y / textureHeight);
  auto texSize = glm::vec2(size.x, textureHeight);
  for (int i = 0; i < numTexPerPage; i++) {
    renderTextures.push_back(std::make_unique<RenderTexture>(texSize, dpiScale, format)
    );
  }
}

// round to prevent floating point errors (very sus but it works)
float ScrollableRenderTexture::RoundOffset(float offset) const {
  return glm::round(offset * dpiScale) / dpiScale;
}

void ScrollableRenderTexture::UpdatePos(glm::vec2 pos) {
  posOffset = pos;
  SetTexturePositions();
  SetTextureCameraPositions();

  if (marginTextures.top != nullptr) {
    marginTextures.top->UpdatePos(posOffset);
  }
  if (marginTextures.bottom != nullptr) {
    marginTextures.bottom->UpdatePos(posOffset + glm::vec2(0, size.y - fmargins.bottom));
  }
}

void ScrollableRenderTexture::UpdateViewport(float newScrollDist) {
  // in the middle of scrolling, instead scroll from current pos to new pos
  if (scrolling) {
    baseOffset += scrollCurr;
    float notScrolled = scrollDist - scrollCurr;
    scrollDist = newScrollDist + notScrolled;

  } else {
    scrollDist = newScrollDist;
  }

  scrolling = true;
  scrollCurr = 0;
  scrollElapsed = 0;

  AddOrRemoveTextures();
  SetTextureCameraPositions();
}

void ScrollableRenderTexture::UpdateScrolling(float dt) {
  scrollElapsed += dt;

  if (scrollElapsed >= scrollTime) {
    baseOffset += scrollDist;
    baseOffset = RoundOffset(baseOffset);

    scrolling = false;
    scrollDist = 0;
    scrollCurr = 0;
    scrollElapsed = 0;

    // AddOrRemoveTextures();
    SetTextureCameraPositions();

  } else {
    float t = scrollElapsed / scrollTime;
    // float y = glm::pow(t, 1 / 2.0f);
    // float y = EaseOutSine(t);
    // float y = EaseOutElastic(t);
    float y = EaseOutQuad(t);

    scrollCurr = glm::sign(scrollDist) * glm::mix(0.0f, glm::abs(scrollDist), y);
  }

  SetTexturePositions();
}

void ScrollableRenderTexture::UpdateMargins(const Margins& newMargins) {
  margins = newMargins;
  fmargins = margins.ToFloat(charSize);

  if (margins.top != 0) {
    glm::vec2 topMarginSize = {size.x, fmargins.top};
    if (marginTextures.top == nullptr || marginTextures.top->size != topMarginSize) {
      marginTextures.top = std::make_unique<RenderTexture>(topMarginSize, dpiScale, format);
      marginTextures.top->UpdatePos(posOffset);
      marginTextures.top->UpdateCameraPos({0, 0});
    }
  }

  if (margins.bottom != 0) {
    glm::vec2 topMarginSize = {size.x, fmargins.bottom};
    if (marginTextures.bottom == nullptr || marginTextures.bottom->size != topMarginSize) {
      marginTextures.bottom = std::make_unique<RenderTexture>(topMarginSize, dpiScale, format);
      marginTextures.bottom->UpdatePos(posOffset + glm::vec2(0, size.y - fmargins.bottom));
      marginTextures.bottom->UpdateCameraPos({0, size.y - fmargins.bottom});
    }
  }
}

void ScrollableRenderTexture::AddOrRemoveTextures() {
  // viewable texture region while incorporating scrolling
  float posChange = (scrollDist > 0 ? 0 : scrollDist);
  Line region{
    .pos = baseOffset + posChange,
    .height = size.y + glm::abs(scrollDist),
  };

  std::vector<RenderTextureHandle> removed;
  // remove from the top
  int numRemoved = 0;
  for (size_t i = 0; i < renderTextures.size(); i++) {
    float currPos = i * textureHeight;
    if (region.Intersects({currPos, textureHeight})) {
      break;
    }
    removed.push_back(std::move(renderTextures[i]));
    numRemoved++;
  }

  renderTextures.erase(renderTextures.begin(), renderTextures.begin() + numRemoved);
  region.pos -= numRemoved * textureHeight;

  // remove from the bottom
  numRemoved = 0;
  for (size_t i = renderTextures.size() - 1; i >= 0; i--) {
    float currPos = i * textureHeight;
    if (region.Intersects({currPos, textureHeight})) {
      break;
    }
    removed.push_back(std::move(renderTextures[i]));
    numRemoved++;
  }
  renderTextures.erase(renderTextures.end() - numRemoved, renderTextures.end());

  auto createHandle = [&removed, this]() {
    if (!removed.empty()) {
      auto handle = std::move(removed.back());
      removed.pop_back();
      return handle;
    }
    auto texSize = glm::vec2(size.x, textureHeight);
    return std::make_unique<RenderTexture>(texSize, dpiScale, format);
  };

  // add from top
  int numAdded = 0;
  for (int i = -1;; i--) {
    float currPos = i * textureHeight;
    if (!region.Intersects({currPos, textureHeight})) {
      break;
    }
    renderTextures.push_front(createHandle());
    numAdded++;
  }
  region.pos += textureHeight * numAdded;

  // add from bottom
  for (size_t i = renderTextures.size();; i++) {
    float currPos = i * textureHeight;
    if (!region.Intersects({currPos, textureHeight})) {
      break;
    }
    renderTextures.push_back(createHandle());
  }

  baseOffset = region.pos - posChange;
}

void ScrollableRenderTexture::SetTexturePositions() {
  for (size_t i = 0; i < renderTextures.size(); i++) {
    auto& texture = *renderTextures[i];
    float yposTop = -(baseOffset + scrollCurr) + (i * textureHeight);
    float yposBottom = yposTop + textureHeight;

    if (yposBottom <= 0 || yposTop >= size.y) {
      texture.disabled = true;
      continue;
    }

    if (yposTop < 0) {
      texture.UpdatePos(
        posOffset,
        Rect{
          .pos = {0, -yposTop},
          .size = {size.x, textureHeight + yposTop},
        }
      );

    } else if (yposBottom > size.y) {
      texture.UpdatePos(
        posOffset + glm::vec2(0, yposTop),
        Rect{
          .pos = {0, 0},
          .size = {size.x, textureHeight - (yposBottom - size.y)},
        }
      );

    } else {
      texture.UpdatePos(posOffset + glm::vec2(0, yposTop));
    }

    texture.disabled = false;
  }
}

void ScrollableRenderTexture::SetTextureCameraPositions() {
  for (size_t i = 0; i < renderTextures.size(); i++) {
    float pos = -(baseOffset + scrollDist) + (i * textureHeight);
    renderTextures[i]->UpdateCameraPos({0, pos});
  }
}

std::vector<RenderInfo> ScrollableRenderTexture::GetRenderInfos() const {
  // top of viewport after scrolling
  float newBaseOffset = baseOffset + scrollDist;
  newBaseOffset = RoundOffset(newBaseOffset);

  int topOffset = newBaseOffset / charSize.y;
  int bottomOffset = (newBaseOffset + size.y) / charSize.y;

  int innerTopOffset = topOffset + margins.top;
  int innerBottomOffset = bottomOffset - margins.bottom;

  int totalRows = size.y / charSize.y;

  std::vector<RenderInfo> renderInfos;

  for (size_t i = 0; i < renderTextures.size(); i++) {
    int top = i * rowsPerTexture;
    int bottom = (i + 1) * rowsPerTexture;

    if (bottom <= innerTopOffset) continue;
    if (top >= innerBottomOffset) break;

    auto& renderInfo = renderInfos.emplace_back(RenderInfo{
      .texture = renderTextures[i].get(),
    });

    int start = glm::max(top - topOffset, margins.top);
    int end = glm::min(bottom - topOffset, totalRows - margins.bottom);
    renderInfo.range = {start, end};

    if (top < innerTopOffset) {
      renderInfo.clearRegion = Rect{
        .pos = {0, start * charSize.y},
        .size =
          {
            size.x,
            (rowsPerTexture - (innerTopOffset - top)) * charSize.y,
          },
      };
    } else if (bottom > innerBottomOffset) {
      renderInfo.clearRegion = Rect{
        .pos = {0, start * charSize.y},
        .size =
          {
            size.x,
            (rowsPerTexture - (bottom - innerBottomOffset)) * charSize.y,
          },
      };
    }
  }

  // margins
  if (marginTextures.top != nullptr) {
    renderInfos.push_back(RenderInfo{
      .texture = marginTextures.top.get(),
      .range = {0, margins.top},
    });
  }
  if (marginTextures.bottom != nullptr) {
    renderInfos.push_back(RenderInfo{
      .texture = marginTextures.bottom.get(),
      .range = {totalRows - margins.bottom, totalRows},
    });
  }

  return renderInfos;
}

void ScrollableRenderTexture::Render(const wgpu::RenderPassEncoder& passEncoder) const {
  if (marginTextures.top != nullptr) {
    passEncoder.SetBindGroup(1, marginTextures.top->textureBG);
    marginTextures.top->renderData.Render(passEncoder);
  }
  if (marginTextures.bottom != nullptr) {
    passEncoder.SetBindGroup(1, marginTextures.bottom->textureBG);
    marginTextures.bottom->renderData.Render(passEncoder);
  }
  for (const auto& renderTexture : renderTextures) {
    if (renderTexture->disabled) continue;
    passEncoder.SetBindGroup(1, renderTexture->textureBG);
    renderTexture->renderData.Render(passEncoder);
  }
}
