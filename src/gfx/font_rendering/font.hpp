#pragma once

#include "./font_descriptor.hpp"
#include "./glyph_info.hpp"
#include "./texture_atlas.hpp"
#include <expected>
#include <string>
#include <stdexcept>

#include <hb-cplusplus.hh>
#include <hb-ft.h>

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
  hb::unique_ptr<hb_font_t> hbFont;
  hb::unique_ptr<hb_buffer_t> hbBuffer;

  std::string path;
  float height;     // font size
  float width;
  float emojiRatio;
  float dpiScale;

  glm::vec2 charSize; // doesn't take in account linespace
  float ascender; // doesn't take in account linespace
  float underlinePosition;
  float underlineThickness;

  using GlyphInfoMap = std::unordered_map<std::string, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;
  GlyphInfoMap emojiGlyphInfoMap;

  static std::expected<Font, std::runtime_error>
  FromName(const FontDescriptorWithName& desc, float dpiScale);

  // static std::expected<Font, std::string>
  // UseEmoji(const FontDescriptorWithName& desc, float linespace, float dpiScale);

  Font(std::string path, float height, float width, float dpiScale);

  // returns nullptr when charcode is not found.
  // updates glyphInfoMap when charcode not in map.
  const GlyphInfo* GetGlyphInfo(
    const std::string& text,
    TextureAtlas<false>& textureAtlas,
    TextureAtlas<true>& colorTextureAtlas
  );
};
