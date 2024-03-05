#include "font.hpp"
#include "webgpu_utils/webgpu.hpp"
#include "gfx/instance.hpp"

#include <iostream>
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

  // winding order is clockwise starting from top left
  GlyphInfo::positions = {
    glm::vec2(0, 0),
    glm::vec2(size, 0),
    glm::vec2(size, size),
    glm::vec2(0, size),
  };

  FT_Set_Pixel_Sizes(face, 0, size * ratio);

  charWidth = (face->size->metrics.max_advance >> 6) / ratio;
  charHeight = (face->size->metrics.height >> 6) / ratio;

  uint32_t numChars = 128;
  atlasHeight = (numChars + atlasWidth - 1) / atlasWidth;

  glm::vec2 textureSize((size + 2) * atlasWidth, (size + 2) * atlasHeight);
  auto bufferSize = textureSize * ratio;

  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> textureData(bufferSize.x * bufferSize.y, {0, 0, 0, 0});

  defaultGlyphIndex = FT_Get_Char_Index(face, ' ');

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

    // region that holds current glyph in context of the entire font texture
    // region = x, y, width, height
    glm::vec4 region{pos.x, pos.y, size, size};

    float left = region.x / textureSize.x;
    float right = (region.x + region.z) / textureSize.x;
    float top = region.y / textureSize.y;
    float bottom = (region.y + region.w) / textureSize.y;

    std::array<glm::vec2, 4> texCoords{
      glm::vec2(left, top),
      glm::vec2(right, top),
      glm::vec2(right, bottom),
      glm::vec2(left, bottom),
    };

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
        .texCoords = texCoords,
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

  fontTextureBG = utils::MakeBindGroup(
    ctx.device, ctx.pipeline.fontTextureBGL,
    {
      {0, texture.view},
      {1, sampler},
    }
  );
}

const Font::GlyphInfo& Font::GetGlyphInfo(FT_ULong charcode) const {
  auto glyphIndex = FT_Get_Char_Index(face, charcode);
  auto it = glyphInfoMap.find(glyphIndex);
  if (it == glyphInfoMap.end()) {
    it = glyphInfoMap.find(defaultGlyphIndex);
  }
  return it->second;
}
