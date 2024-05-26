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
    std::mdspan(dataRaw.data(), std::dextents<uint, 2>{bufferSize.y, bufferSize.x});

  // init bind group data
  textureSizeBuffer =
    utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

  texture = utils::CreateBindingTexture(
    ctx.device, Extent3D(bufferSize.x, bufferSize.y), wgpu::TextureFormat::RGBA8UnormSrgb
  );

  textureSampler = ctx.device.CreateSampler(ToPtr(SamplerDescriptor{
    .addressModeU = AddressMode::ClampToEdge,
    .addressModeV = AddressMode::ClampToEdge,
    .magFilter = FilterMode::Nearest,
    .minFilter = FilterMode::Nearest,
  }));

  fontTextureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.fontTextureBGL,
    {
      {0, textureSizeBuffer},
      {1, texture.CreateView()},
      {2, textureSampler},
    }
  );
}

Region TextureAtlas::AddGlyph(std::mdspan<uint8_t, std::dextents<uint, 2>> glyphData) {
  // check if current row is full
  // if so, move to next row
  if (currentPos.x + glyphData.extent(1) > bufferSize.x) {
    currentPos.x = 0;
    currentPos.y += currMaxHeight;
    currMaxHeight = 0;
  }
  if (currentPos.y + glyphData.extent(0) > bufferSize.y) {
    Resize();
  }

  // fill data
  for (uint row = 0; row < glyphData.extent(0); row++) {
    for (uint col = 0; col < glyphData.extent(1); col++) {
      Color& dest = data[currentPos.y + row, currentPos.x + col];
      dest.r = 255;
      dest.g = 255;
      dest.b = 255;
      dest.a = glyphData[row, col];
    }
  }
  dirty = true;

  // calculate region
  auto regionPos = glm::vec2(currentPos) / dpiScale;
  auto regionSize = glm::vec2(glyphData.extent(1), glyphData.extent(0)) / dpiScale;

  // advance current position
  currentPos.x += glyphData.extent(1);
  currMaxHeight = std::max(currMaxHeight, glyphData.extent(0));

  return MakeRegion(regionPos, regionSize);
}

void TextureAtlas::Resize() {
  uint heightIncrease = trueGlyphSize * 3;
  bufferSize.y += heightIncrease;
  textureSize = glm::vec2(bufferSize) / dpiScale;

  dataRaw.resize(bufferSize.x * bufferSize.y);
  data =
    std::mdspan(dataRaw.data(), std::dextents<uint, 2>{bufferSize.y, bufferSize.x});

  resized = true;
}

void TextureAtlas::Update() {
  if (!dirty) return;

  if (resized) {
    // replace buffer because we dont want to change previous buffer data
    // we wanna create copy of it so different instances of the buffer
    // can be used in the same command encoder
    textureSizeBuffer =
      utils::CreateUniformBuffer(ctx.device, sizeof(glm::vec2), &textureSize);

    texture = utils::CreateBindingTexture(
      ctx.device, Extent3D(bufferSize.x, bufferSize.y), wgpu::TextureFormat::RGBA8UnormSrgb
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
