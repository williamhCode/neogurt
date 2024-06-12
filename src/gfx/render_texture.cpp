#include "render_texture.hpp"

#include "webgpu_tools/utils/webgpu.hpp"
#include "utils/region.hpp"
#include "gfx/instance.hpp"
#include <algorithm>

using namespace wgpu;

RenderTexture::RenderTexture(
  glm::vec2 _size, float dpiScale, wgpu::TextureFormat format, const void* data
)
    : size(_size) {
  camera = Ortho2D(size);

  auto fbSize = size * dpiScale;
  texture =
    utils::CreateRenderTexture(ctx.device, Extent3D(fbSize.x, fbSize.y), format, data);
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

void RenderTexture::UpdatePos(glm::vec2 pos, Rect* region) {
  renderData.ResetCounts();

  Region positions;
  Region uvs;

  if (region == nullptr) {
    positions = MakeRegion(pos, size);
    uvs = MakeRegion({0, 0}, {1, 1});
  } else {
    positions = MakeRegion(pos, region->size);
    region->pos /= size, region->size /= size;
    uvs = region->Region();
  }

  for (size_t i = 0; i < 4; i++) {
    auto& vertex = renderData.CurrQuad()[i];
    vertex.position = positions[i];
    vertex.uv = uvs[i];
  }
  renderData.Increment();
  renderData.WriteBuffers();
}

// ------------------------------------------------------------------
ScrollableRenderTexture::ScrollableRenderTexture(glm::vec2 _size, float _dpiScale)
    : size(_size), dpiScale(_dpiScale) {
  // initial page of textures
  for (int i = 0; i < numTexPerPage; i++) {
    renderTextures.emplace_back(size, dpiScale, format);
  }

  textureHeight = size.y / numTexPerPage;
}

void ScrollableRenderTexture::UpdatePos(glm::vec2 _pos) {
  pos = _pos;

  // TODO
}

void ScrollableRenderTexture::UpdateViewport(float newScrollDist) {
  // not scrolling, reset view and return
  if (newScrollDist == 0) {
    scrolling = false;
    // scrollDist = 0;
    // scrollCurr = 0;
    // scrollElapsed = 0;
    baseOffset = 0;

  } else {
    // in the middle of scrolling, instead scroll from current pos to new pos
    if (scrolling) {
      baseOffset += scrollCurr;
      float notScrolled = scrollDist - scrollCurr;
      scrollDist = newScrollDist + notScrolled;
    } else {
      scrollDist = newScrollDist;
    }

    scrolling = true;
    scrollCurr = 0; // probably not necessary, will be overridden when updating
    scrollElapsed = 0;
  }

  AddOrRemoveTextures();
}

void ScrollableRenderTexture::AddOrRemoveTextures() {
  glm::vec2 pos(0, baseOffset);
  glm::vec2 size(this->size.x, textureHeight);

  if (scrollDist > 0) { // texture moving up
    size.y += scrollDist;

  } else if (scrollDist < 0) { // texture moving down
    pos.y += scrollDist;
    size.y -= scrollDist;
  }

  // if (!scrolling) {
  //   // remove from front
  //   int numToRemove = baseOffset / textureHeight;
  //   baseOffset -= numToRemove * textureHeight;
  //   renderTextures.erase(renderTextures.begin(), renderTextures.begin() +
  //   numToRemove);

  //   // remove from end
  //   int numToKeep = numTexPerPage + (baseOffset == 0 ? 0 : 1);
  //   renderTextures.erase(renderTextures.begin() + numToKeep, renderTextures.end());

  // } else {
  //   if (scrollDist > 0) { // texture moving up
  //     int numToRemove = baseOffset / textureHeight;
  //     // remove from front, but store in temporary vector
  //     std::vector<RenderTextureHandle> removed(
  //       std::make_move_iterator(renderTextures.begin()),
  //       std::make_move_iterator(renderTextures.begin() + numToRemove)
  //     );
  //     renderTextures.erase(
  //       renderTextures.begin(), renderTextures.begin() + numToRemove
  //     );

  //     baseOffset -= numToRemove * textureHeight;
  //     float newBaseOffset = baseOffset + scrollDist;

  //     // add bottom textures
  //     float bottomOffset = newBaseOffset + size.y;
  //     int numToAdd = int(bottomOffset / textureHeight) - renderTextures.size();
  //     if (numToAdd > 0) {
  //       for (int i = 0; i < numToAdd; i++) {
  //         if (removed.empty()) {
  //           renderTextures.emplace_back(size, dpiScale, format);
  //         } else {
  //           // reuse removed textures
  //           renderTextures.push_back(std::move(removed.back()));
  //           removed.pop_back();
  //         }
  //       }
  //     } else if (numToAdd < 0) {
  //       renderTextures.erase(renderTextures.end() + numToAdd, renderTextures.end());
  //     }

  //   } else { // texture moving down
  //     float bottomOffset = baseOffset + size.y;
  //     int numToRemove = renderTextures.size() - int(bottomOffset / textureHeight);
  //     // remove from end, but store in temporary vector
  //     std::vector<RenderTextureHandle> removed(
  //       std::make_move_iterator(renderTextures.end() - numToRemove),
  //       std::make_move_iterator(renderTextures.end())
  //     );
  //     renderTextures.erase(renderTextures.end() - numToRemove, renderTextures.end());

  //     float newBaseOffset = baseOffset + scrollDist;

  //     // add top textures

  //   }
  // }
}

void ScrollableRenderTexture::UpdateScrolling(float dt) {
}
