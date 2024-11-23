#pragma once

#include "gfx/font/descriptor.hpp"
#include "gfx/texture_atlas.hpp"
#include "gfx/glyph_info.hpp"
#include <expected>

#include <ft2build.h>
#include <freetype/freetype.h>

int FtInit();
void FtDone();

struct FT_FaceDeleter {
  void operator()(FT_Face face) {
    FT_Done_Face(face);
  }
};
using FT_FacePtr = std::unique_ptr<FT_FaceRec, FT_FaceDeleter>;

struct Font {
  FT_FacePtr face;

  std::string path;
  float height;     // font size
  float width;
  float linespace;
  float dpiScale;

  glm::vec2 charSize;
  float ascender;
  float underlinePosition;
  float underlineThickness;

  // index is FT glyph index, not charcode
  using GlyphInfoMap = std::unordered_map<FT_UInt, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;

  static std::expected<Font, std::string>
  FromName(const FontDescriptorWithName& desc, float linespace, float dpiScale);

  Font(std::string path, float height, float width, float linespace, float dpiScale);

  // returns nullptr when charcode is not found.
  // updates glyphInfoMap when charcode not in map.
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas);
};
