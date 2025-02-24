#include "./texture_atlas.hpp"
#include "gfx/instance.hpp"

using namespace wgpu;

template <bool IsColor>
TextureAtlas<IsColor>::TextureAtlas(float _height, float _dpiScale)
    : dpiScale(_dpiScale), trueHeight(_height * dpiScale) {

  int initialHeight = trueHeight * 3;
  int initialWidth = trueHeight * glyphsPerRow;
  initialWidth = std::min<int>(initialWidth, ctx.limits.maxTextureDimension2D);
  bufferSize = {initialWidth, initialHeight};
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data = std::mdspan(dataRaw.data(), bufferSize.y, bufferSize.x);

  // init bind group data
  textureSizeBuffer = ctx.CreateUniformBuffer(sizeof(glm::vec2), &textureSize);

  textureSizeBG = ctx.MakeBindGroup(
    ctx.pipeline.textureSizeBGL,
    {
      {0, textureSizeBuffer},
    }
  );

  if constexpr (IsColor) {
    renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::RGBA8Unorm);
  } else {
    renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::R8Unorm);
  }
}

template <bool IsColor>
void TextureAtlas<IsColor>::Resize() {
  int heightIncrease = trueHeight * 3;
  int newBufferHeight = bufferSize.y + heightIncrease;
  if (newBufferHeight > ctx.limits.maxTextureDimension2D) {
    // TODO: implement LRU cache
    throw std::runtime_error("TextureAtlas: Texture size limit reached");
  } else {
    bufferSize.y = newBufferHeight;
  }
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data = std::mdspan(dataRaw.data(), bufferSize.y, bufferSize.x);

  resized = true;
  // LOG_INFO("Resized texture atlas to {}x{}", bufferSize.x, bufferSize.y);
}

template <bool IsColor>
void TextureAtlas<IsColor>::Update() {
  if (!dirty) return;

  if (resized) {
    // replace buffer because we dont want to change previous buffer data
    // we wanna create copy of it so different instances of the buffer
    // can be used in the same command encoder
    textureSizeBuffer = ctx.CreateUniformBuffer(sizeof(glm::vec2), &textureSize);

    textureSizeBG = ctx.MakeBindGroup(
      ctx.pipeline.textureSizeBGL,
      {
        {0, textureSizeBuffer},
      }
    );

    if constexpr (IsColor) {
      renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::RGBA8Unorm);
    } else {
      renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::R8Unorm);
    }
    resized = false;
  }

  ctx.WriteTexture(renderTexture.texture, bufferSize, dataRaw.data());
  dirty = false;
}

// explicit template instantiation
template struct TextureAtlas<true>;
template struct TextureAtlas<false>;

