#pragma once

#include "utils/region.hpp"
#include "webgpu/webgpu_cpp.h"
#include "gfx/camera.hpp"
#include "gfx/quad.hpp"
#include "glm/ext/vector_float2.hpp"
#include <memory>

struct RenderTexture {
  Ortho2D camera;

  glm::vec2 size;
  wgpu::Texture texture;
  wgpu::TextureView textureView;
  wgpu::BindGroup textureBG;

  QuadRenderData<TextureQuadVertex> renderData;

  RenderTexture() = default;
  RenderTexture(
    glm::vec2 size,
    float dpiScale,
    wgpu::TextureFormat format,
    const void* data = nullptr
  );

  // pos is the position (top left) of the texture in the screen
  // region is the subregion of the texture to draw
  void UpdatePos(glm::vec2 pos, Rect* region = nullptr);
};

using RenderTextureHandle = std::unique_ptr<RenderTexture>;

// consists of multiple render textures that can be scrolled
struct ScrollableRenderTexture {
  glm::vec2 size;
  float dpiScale;
  float textureHeight;

  glm::vec2 pos;

  int numTexPerPage = 2;
  wgpu::TextureFormat format = wgpu::TextureFormat::BGRA8UnormSrgb;

  // dynamic variables
  std::deque<RenderTextureHandle> renderTextures;
  float baseOffset = 0; // offset representing top of viewport

  bool scrolling;
  float scrollDist; // baseOffset + scrollDist = new baseOffset
  float scrollCurr; // 0 <= scrollCurr <= scrollDist
  float scrollTime = 0.1; // transition time
  float scrollElapsed;

  ScrollableRenderTexture() = default;
  ScrollableRenderTexture(
    glm::vec2 size,
    float dpiScale
    // int numTexPerPage,
    // wgpu::TextureFormat format
  );
    
  void UpdatePos(glm::vec2 pos);
  void UpdateViewport(float newScrollDist = 0);
  void AddOrRemoveTextures();
  void UpdateScrolling(float dt);

  float GetBottomOffset(float topOffset);
};
