#include "font.hpp"
#include "dawn/utils/WGPUHelpers.h"
#include "webgpu_utils/webgpu.hpp"
#include "gfx/instance.hpp"

#include <vector>

static FT_Library library;
static bool ftInitialized = false;

using namespace wgpu;

Font::Font(const std::string& path, int _size, float ratio) : size(_size) {
  if (!ftInitialized) {
    if (FT_Init_FreeType(&library)) {
      throw std::runtime_error("Failed to initialize FreeType library");
    }
    ftInitialized = true;
  }

  if (FT_New_Face(library, path.c_str(), 0, &face)) {
    throw std::runtime_error("Failed to load font");
  }

  FT_Set_Pixel_Sizes(face, 0, size * ratio);

  uint32_t numChars = 128;
  atlasHeight = numChars / atlasWidth;

  glm::vec2 textureSize((size + 2) * atlasWidth, (size + 2) * atlasHeight);
  auto bufferSize = textureSize * ratio;

  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> textureData(bufferSize.x * bufferSize.y, {0, 0, 0, 0});

  // start off by rendering the first 128 characters
  for (uint32_t i = 0; i < numChars; i++) {
    auto glyphIndex = FT_Get_Char_Index(face, i);
    // if (glyph_index == 0) {
    //   throw std::runtime_error(std::format("Failed to load glyph for character {}",
    //   i));
    // }
    FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);

    glm::vec2 pos{
      (i % atlasWidth) * (size + 2) + 1,
      (i / atlasWidth) * (size + 2) + 1,
    };
    auto truePos = pos * ratio;

    auto& glyph = *(face->glyph);
    auto& bitmap = glyph.bitmap;
    for (size_t yy = 0; yy < bitmap.rows; yy++) {
      for (size_t xx = 0; xx < bitmap.width; xx++) {
        auto index = truePos.x + xx + (truePos.y + yy) * bufferSize.x;
        textureData[index].r = 255;
        textureData[index].g = 255;
        textureData[index].b = 255;
        textureData[index].a = bitmap.buffer[xx + yy * bitmap.width];
      }
    }

    glyphInfoMap.emplace(
      glyphIndex,
      GlyphInfo{
        .size =
          {
            bitmap.width / ratio,
            bitmap.rows / ratio,
          },
        .bearing =
          {
            glyph.bitmap_left / ratio,
            glyph.bitmap_top / ratio,
          },
        .advance = (glyph.advance.x >> 6) / ratio,
        .pos = pos,
      }
    );
  }

  texture = TextureHandle{
    .t = utils::CreateTexture(
      ctx.device,
      {static_cast<uint32_t>(bufferSize.x), static_cast<uint32_t>(bufferSize.y)},
      wgpu::TextureFormat::RGBA8Unorm, textureData.data()
    ),
    .width = static_cast<int>(textureSize.x),
    .height = static_cast<int>(textureSize.y),
  };
  texture.CreateView();

  // create bind group
  Sampler sampler = ctx.device.CreateSampler( //
    ToPtr(SamplerDescriptor{
      .addressModeU = AddressMode::ClampToEdge,
      .addressModeV = AddressMode::ClampToEdge,
      .magFilter = FilterMode::Nearest,
      .minFilter = FilterMode::Nearest,
    })
  );

  fontTextureBG = dawn::utils::MakeBindGroup(
    ctx.device, ctx.pipeline.fontTextureBGL,
    {
      {0, texture.view},
      {1, sampler},
    }
  );
}
