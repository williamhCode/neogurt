#include "texture_atlas.hpp"
#include "utils/region.hpp"
#include "gfx/instance.hpp"
#include "webgpu_tools/utils/webgpu.hpp"

using namespace wgpu;

TextureAtlas::TextureAtlas(float _glyphSize, float _dpiScale)
    : dpiScale(_dpiScale), trueGlyphSize(_glyphSize * dpiScale) {

  int initialTextureHeight = trueGlyphSize * 3;
  bufferSize = {trueGlyphSize * glyphsPerRow, initialTextureHeight};
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data = std::mdspan(dataRaw.data(), bufferSize.y, bufferSize.x);

  // init bind group data
  textureSizeBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

  textureSizeBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.textureSizeBGL,
    {
      {0, textureSizeBuffer},
    }
  );

  renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::RGBA8Unorm);
}

void TextureAtlas::Resize() {
  int heightIncrease = trueGlyphSize * 3;
  bufferSize.y += heightIncrease;
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data = std::mdspan(dataRaw.data(), bufferSize.y, bufferSize.x);

  resized = true;
  // LOG_INFO("Resized texture atlas to {}x{}", bufferSize.x, bufferSize.y);
}

void TextureAtlas::Update() {
  if (!dirty) return;

  if (resized) {
    // replace buffer because we dont want to change previous buffer data
    // we wanna create copy of it so different instances of the buffer
    // can be used in the same command encoder
    textureSizeBuffer =
      utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

    textureSizeBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.textureSizeBGL,
      {
        {0, textureSizeBuffer},
      }
    );

    renderTexture = RenderTexture(textureSize, dpiScale, TextureFormat::RGBA8Unorm);
    resized = false;
  }

  utils::WriteTexture(ctx.device, renderTexture.texture, bufferSize, dataRaw.data());
  dirty = false;
}
