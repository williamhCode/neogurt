#include "./texture_atlas.hpp"
#include "gfx/instance.hpp"
#include "utils/logger.hpp"
#include <utility>

using namespace wgpu;

template <bool IsColor>
TextureAtlas<IsColor>::TextureAtlas(float _glyphSize, float _dpiScale)
    : glyphSize(_glyphSize), dpiScale(_dpiScale), trueHeight(glyphSize * dpiScale) {

  uint initialHeight = trueHeight * 3;
  uint initialWidth = trueHeight * glyphsPerRow;
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

  renderTexture = RenderTexture(textureSize, dpiScale, textureFormat);
}

template <bool IsColor>
void TextureAtlas<IsColor>::Resize() {
  int heightIncrease = trueHeight * 3;
  uint newBufferHeight = bufferSize.y + heightIncrease;
  uint textureBytes = bufferSize.x * newBufferHeight * sizeof(Pixel);

  // return true if texture atlas if we exceed the max texture size or taking too much memory
  if (newBufferHeight > ctx.limits.maxTextureDimension2D || textureBytes > maxBytes) {
    throw TextureResizeError{
      IsColor ? TextureResizeError::Colored : TextureResizeError::Normal
    };
  }

  bufferSize.y = newBufferHeight;
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

    renderTexture = RenderTexture(textureSize, dpiScale, textureFormat);

    resized = false;
  }

  ctx.WriteTexture(renderTexture.texture, bufferSize, dataRaw.data());
  dirty = false;
}

// explicit template instantiation
template struct TextureAtlas<true>;
template struct TextureAtlas<false>;

