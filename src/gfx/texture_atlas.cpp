#include "texture_atlas.hpp"
#include "utils/region.hpp"
#include "gfx/instance.hpp"
#include "webgpu_tools/utils/webgpu.hpp"

using namespace wgpu;

TextureAtlas::TextureAtlas(uint _glyphSize, float _dpiScale)
    : dpiScale(_dpiScale), trueGlyphSize(_glyphSize * dpiScale) {

  uint initialTextureHeight = trueGlyphSize * 3;
  bufferSize = {trueGlyphSize * glyphsPerRow, initialTextureHeight};
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data =
    std::mdspan(dataRaw.data(), std::dextents<uint, 2>{bufferSize.x, bufferSize.y});

  // init bind group data
  textureSizeBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

  texture = utils::CreateBindingTexture(
    ctx.device, Extent3D(bufferSize.x, bufferSize.y), wgpu::TextureFormat::RGBA8Unorm
  );

  textureSampler = ctx.device.CreateSampler(
    ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    })
  );

  fontTextureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.fontTextureBGL,
    {
      {0, textureSizeBuffer},
      {1, texture.CreateView()},
      {2, textureSampler},
    }
  );
}

Region TextureAtlas::AddGlyph(std::mdspan<Color, std::dextents<uint, 2>> glyphData) {
  // check if current row is full
  // if so, move to next row
  if (currentPos.x + glyphData.extent(0) > bufferSize.x) {
    currentPos.x = 0;
    currentPos.y += currMaxHeight;
    currMaxHeight = 0;

    if (currentPos.y + glyphData.extent(1) > bufferSize.y) {
      Resize();
    }
  }

  // fill data
  for (uint row = 0; row < glyphData.extent(1); row++) {
    for (uint col = 0; col < glyphData.extent(0); col++) {
      Color& dest = data[row + currentPos.y, col + currentPos.x];
      Color& src = glyphData[row, col];
      dest = src;
    }
  }
  dirty = true;

  // calculate region
  auto regionPos = glm::vec2(currentPos) / dpiScale;
  auto regionSize = glm::vec2(glyphData.extent(0), glyphData.extent(1)) / dpiScale;

  // advance current position
  currentPos.x += glyphData.extent(0);
  currMaxHeight = std::max(currMaxHeight, glyphData.extent(1));

  return MakeRegion(regionPos, regionSize);
}

void TextureAtlas::Resize() {
  uint heightIncrease = trueGlyphSize * 3;
  bufferSize.y += heightIncrease;
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data =
    std::mdspan(dataRaw.data(), std::dextents<uint, 2>{bufferSize.x, bufferSize.y});

  resized = true;
}

void TextureAtlas::Update() {
  if (!dirty) return;

  if (resized) {
    ctx.queue.WriteBuffer(textureSizeBuffer, 0, &textureSize, sizeof(glm::vec2));

    texture = utils::CreateBindingTexture(
      ctx.device, Extent3D(bufferSize.x, bufferSize.y), wgpu::TextureFormat::RGBA8Unorm
    );

    fontTextureBG = utils::MakeBindGroup(
      ctx.device, ctx.pipeline.fontTextureBGL,
      {
        {0, textureSizeBuffer},
        {1, texture.CreateView()},
        {2, textureSampler},
      }
    );
    resized = false;
  }

  utils::WriteTexture(
    ctx.device, texture, Extent3D(bufferSize.x, bufferSize.y), dataRaw.data()
  );
  dirty = false;
}

