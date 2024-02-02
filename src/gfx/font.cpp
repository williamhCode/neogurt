#include "font.hpp"
#include "utils/webgpu.hpp"
#include "gfx/instance.hpp"

#include "freetype/freetype.h"
#include <format>
#include <iostream>
#include <vector>

Font::Font(const std::string& path, int size, int ratio) : size(size) {
  FT_Library library;
  if (FT_Init_FreeType(&library)) {
    throw std::runtime_error("Failed to initialize FreeType library");
  }

  FT_Face face;
  if (FT_New_Face(library, path.c_str(), 0, &face)) {
    throw std::runtime_error("Failed to load font");
  }

  FT_Set_Pixel_Sizes(face, 0, size * ratio);

  uint32_t numChars = 128;
  atlasHeight = numChars / atlasWidth;

  int textureWidth = (size + 2) * atlasWidth;
  int textureHeight = (size + 2) * atlasHeight;
  textureWidth *= ratio;
  textureHeight *= ratio;

  struct Color {
    uint8_t r, g, b, a;
  };
  std::vector<Color> textureData(textureWidth * textureHeight, {0, 0, 0, 0});

  // stat off by rendering the first 128 characters
  for (uint32_t i = 0; i <= numChars; i++) {
    auto glyph_index = FT_Get_Char_Index(face, i);
    if (glyph_index == 0) {
      throw std::runtime_error(std::format("Failed to load glyph for character {}", i));
    }
    FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);

    glm::ivec2 pos{
      (i % atlasWidth) * (size + 2) + 1,
      (i / atlasWidth) * (size + 2) + 1,
    };
    auto truePos = pos * ratio;

    auto& glyph = *(face->glyph);
    auto& bitmap = glyph.bitmap;
    for (size_t yy = 0; yy < bitmap.rows; yy++) {
      for (size_t xx = 0; xx < bitmap.width; xx++) {
        auto index = truePos.x + xx + (truePos.y + yy) * textureWidth;
        textureData[index].r = 255;
        textureData[index].g = 255;
        textureData[index].b = 255;
        textureData[index].a = bitmap.buffer[xx + yy * bitmap.width];
      }
    }

    glyphInfoMap.emplace(glyph_index, GlyphInfo{
      .size = {
        bitmap.width / ratio,
        bitmap.rows / ratio,
      },
      .bearing = {
        glyph.bitmap_left / ratio,
        glyph.bitmap_top / ratio,
      },
      .advance = static_cast<int>((glyph.advance.x >> 6) / ratio),
      .position = pos,
    });
  }

  wgpu::Extent3D textureSize{
    static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight)
  };
  texture = utils::CreateTexture(
    ctx.device, textureSize, wgpu::TextureFormat::RGBA8Unorm, textureData.data()
  );
}
