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
#include <hb-coretext.h>

#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>

// No-ops for compatibility with FreeType init/done call sites
inline int FtInit() { return 0; }
inline void FtDone() {}

struct CTFontDeleter {
  void operator()(CTFontRef font) const {
    if (font) CFRelease(font);
  }
};
using CTFontPtr = std::unique_ptr<std::remove_pointer_t<CTFontRef>, CTFontDeleter>;

struct Font {
  CTFontPtr ctFont;
  hb::unique_ptr<hb_font_t> hbFont;
  hb::unique_ptr<hb_buffer_t> hbBuffer;

  std::string path;
  std::string familyName;
  float height;
  float width;
  float dpiScale;

  bool isColorFont; // true if font has color glyphs (e.g. Apple Color Emoji)

  glm::vec2 charSize;
  float ascender;
  float underlinePosition;
  float underlineThickness;
  float strikeoutPosition;
  float strikeoutThickness;

  std::set<std::string> supportedTexts;

  using GlyphInfoMap = std::unordered_map<uint32_t, GlyphInfo>;
  GlyphInfoMap glyphInfoMap;
  GlyphInfoMap emojiGlyphInfoMap;

  std::vector<hb_feature_t> features;

  // Reusable scratch buffers for RasterizeGlyph (avoids per-glyph allocation)
  std::vector<uint8_t> glyphBuf;
  std::vector<uint8_t> glyphColorBuf;

  static std::expected<Font, std::runtime_error>
  FromName(const FontDescriptorWithName& desc, float dpiScale);

  Font(std::string path, float height, float width, float dpiScale);

  void SetFeatures(std::string_view featuresStr);

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
