#pragma once

#include "glm/ext/vector_uint2.hpp"
#include "utils/margins.hpp"
#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include "gfx/camera.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include <memory>
#include <optional>
#include <tuple>

struct RenderTexture {
  Ortho2D camera;

  glm::vec2 size;
  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> renderData;

  bool disabled = false;

  RenderTexture() = default;
  RenderTexture(
    glm::vec2 size,
    float dpiScale,
    wgpu::TextureFormat format,
    const void* data = nullptr
  );

  // pos is the position (top left) of the texture in the screen
  // region is the subregion of the texture to draw
  void UpdatePos(glm::vec2 pos, std::optional<Rect> region = std::nullopt);
  void UpdateCameraPos(glm::vec2 pos);
};

using RenderTextureHandle = std::unique_ptr<RenderTexture>;

struct RenderInfo {
  const RenderTexture* texture;
  struct {
    int start;
    int end;
  } range;
  std::optional<Rect> clearRegion;
};

struct MarginTextures {
  RenderTextureHandle top;
  RenderTextureHandle bottom;
  RenderTextureHandle left;
  RenderTextureHandle right;
};

// consists of multiple render textures that can be scrolled
struct ScrollableRenderTexture {
  glm::vec2 posOffset;
  glm::vec2 size; // displayable size
  float dpiScale;

  glm::vec2 charSize;

  int maxNumTexPerPage = 2;
  float textureHeight;
  int rowsPerTexture;

  wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8UnormSrgb;

  // dynamic variables
  Margins margins;
  FMargins fmargins;
  MarginTextures marginTextures;

  std::deque<RenderTextureHandle> renderTextures;
  float baseOffset = 0; // offset representing top of viewport (prescroll)

  bool scrolling = false;
  float scrollDist = 0; // baseOffset + scrollDist = new baseOffset
  float scrollCurr = 0; // 0 <= scrollCurr <= scrollDist
  float scrollElapsed = 0;
  float scrollTime = 0.25; // transition time

  ScrollableRenderTexture() = default;
  ScrollableRenderTexture(glm::vec2 size, float dpiScale, glm::vec2 charSize);

  float RoundOffset(float offset) const;

  void UpdatePos(glm::vec2 pos);
  void UpdateViewport(float newScrollDist = 0);
  void UpdateScrolling(float dt);
  void UpdateMargins(const Margins& newMargins);

  void AddOrRemoveTextures();
  void SetTexturePositions();
  void SetTextureCameraPositions();

  // returns pointer to render texture and a row range to render to it
  std::vector<RenderInfo> GetRenderInfos() const;

  // render entire scrollable render texture
  void Render(const wgpu::RenderPassEncoder& passEncoder) const;
};
