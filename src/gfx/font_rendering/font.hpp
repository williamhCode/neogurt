#pragma once

#include "./font_descriptor.hpp"
#include "./glyph_info.hpp"
#include "./texture_atlas.hpp"
#include "utils/logger.hpp"
#include <expected>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

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
  std::string familyName;
  float height;     // font size
  float width;
  float dpiScale;
  float emojiRatio;

  glm::vec2 charSize; // doesn't take in account linespace
  float ascender; // doesn't take in account linespace
  float underlinePosition;
  float underlineThickness;
  float strikeoutPosition;
  float strikeoutThickness;

 // for quick lookup of whether a char is supported by this font
  std::set<std::string> supportedTexts;

  using GlyphInfoMap = std::unordered_map<uint32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;
  GlyphInfoMap emojiGlyphInfoMap;

  // OpenType font features (e.g., stylistic sets, character variants)
  std::vector<hb_feature_t> features;

  void SetFeatures(std::string_view featuresStr);

  static std::expected<Font, std::runtime_error>
  FromName(const FontDescriptorWithName& desc, float dpiScale);

  Font(std::string path, float height, float width, float dpiScale);

  bool ShouldRenderText(const std::string& text);

  std::vector<ShapedGlyph> ShapeText(
    const std::string& text,
    TextureAtlas<false>& textureAtlas,
    TextureAtlas<true>& colorTextureAtlas
  );

  // private
  GlyphInfo* RasterizeGlyph(
    uint32_t glyphIndex,
    TextureAtlas<false>& textureAtlas,
    TextureAtlas<true>& colorTextureAtlas
  );
};
