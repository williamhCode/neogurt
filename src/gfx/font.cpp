#include "font.hpp"
#include "utils/logger.hpp"
#include "webgpu_utils/webgpu.hpp"
#include "gfx/instance.hpp"

#include <vector>

using namespace wgpu;

FT_Library library;
bool ftInitialized = false;

FT_Face nerdFace;

Font::Font(const std::string& path, int _size, float _ratio)
    : size(_size), ratio(_ratio) {
  if (!ftInitialized) {
    if (FT_Init_FreeType(&library)) {
      throw std::runtime_error("Failed to initialize FreeType library");
    }
    ftInitialized = true;

    std::string nerdFontPath(
      ROOT_DIR "/res/Hack/HackNerdFont-Regular.ttf"
      // ROOT_DIR "/res/Hack/HackNerdFontMono-Regular.ttf"
    );
    if (FT_New_Face(library, nerdFontPath.c_str(), 0, &nerdFace)) {
      throw std::runtime_error("Failed to load nerd font");
    }
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

  // defaultGlyphIndex = FT_Get_Char_Index(face, ' ');

  // start off by rendering the first 128 characters
  for (uint32_t i = 0; i < numChars; i++) {
    AddOrGetGlyphInfo(i);
  }

  UpdateTexture();
}

Font::GlyphInfoMap::iterator Font::AddOrGetGlyphInfo(FT_ULong charcode) {
  auto glyphIndex = FT_Get_Char_Index(face, charcode);

  FT_Face currFace = face;
  GlyphInfoMap* currMap = &glyphInfoMap;
  // is nerd font or undefined
  if (glyphIndex == 0) {
    glyphIndex = FT_Get_Char_Index(nerdFace, charcode);
    auto it = nerdGlyphInfoMap.find(glyphIndex);
    if (it != nerdGlyphInfoMap.end()) {
      return it;
    }

    currFace = nerdFace;
    currMap = &nerdGlyphInfoMap;

    FT_Set_Pixel_Sizes(nerdFace, 0, size * ratio);
  } else {
    auto it = glyphInfoMap.find(glyphIndex);
    if (it != glyphInfoMap.end()) {
      return it;
    }
  }

  // LOG("adding new glyph: {}", glyphIndex);
  dirty = true;

  auto numGlyphs = glyphInfoMap.size() + nerdGlyphInfoMap.size() + 1;
  atlasHeight = (numGlyphs + atlasWidth - 1) / atlasWidth;

  textureSize = {(size + 2) * atlasWidth, (size + 2) * atlasHeight};
  bufferSize = textureSize * ratio;

  textureData.resize(bufferSize.x * bufferSize.y, {0, 0, 0, 0});

  auto index = numGlyphs - 1;

  FT_Load_Glyph(currFace, glyphIndex, FT_LOAD_RENDER);
  auto& glyph = *(currFace->glyph);
  auto& bitmap = glyph.bitmap;

  glm::vec2 pos{
    (index % atlasWidth) * (size + 2) + 1,
    (index / atlasWidth) * (size + 2) + 1,
  };
  auto truePos = pos * ratio;

  for (size_t yy = 0; yy < bitmap.rows; yy++) {
    for (size_t xx = 0; xx < bitmap.width; xx++) {
      auto index = truePos.x + xx + (truePos.y + yy) * bufferSize.x;
      textureData[index].r = 255;
      textureData[index].g = 255;
      textureData[index].b = 255;
      textureData[index].a = bitmap.buffer[xx + yy * bitmap.width];
    }
  }

  auto pair = currMap->emplace(
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

  return pair.first;
}

const Font::GlyphInfo& Font::GetGlyphInfo(FT_ULong charcode) {
  return AddOrGetGlyphInfo(charcode)->second;
}

void Font::UpdateTexture() {
  if (!dirty) return;

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

  dirty = false;
}
