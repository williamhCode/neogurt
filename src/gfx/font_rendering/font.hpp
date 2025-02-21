#pragma once

#include "./font_descriptor.hpp"
#include "./glyph_info.hpp"
#include "./texture_atlas.hpp"
#include <expected>

#include <ft2build.h>
#include FT_FREETYPE_H

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

  static std::expected<Font, std::string>
  UseEmoji(const FontDescriptorWithName& desc, float linespace, float dpiScale);

  Font(std::string path, float height, float width, float linespace, float dpiScale);

  // returns nullptr when charcode is not found.
  // updates glyphInfoMap when charcode not in map.
  const GlyphInfo* GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas);
};
