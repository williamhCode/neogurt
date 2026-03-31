#pragma once

#include "editor/highlight.hpp"
#include "gfx/font_rendering/texture_atlas.hpp"
#include "gfx/font_rendering/font.hpp"
#include "gfx/font_rendering/shape_drawing.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <expected>
#include <unordered_set>
#include <optional>
#include <ranges>

using FontHandle = std::shared_ptr<Font>;
// Font is shared with normal if bold/italic/boldItalic is not available
// If variation exists, it owns its own Font
// Fields are never null
struct FontSet {
  FontHandle normal;
  FontHandle bold;
  FontHandle italic;
  FontHandle boldItalic;

  std::vector<const FontHandle*> UniqueFonts() const {
    std::vector<const FontHandle*> result = {&normal};
    for (const FontHandle* fh : {&bold, &italic, &boldItalic}) {
      if (*fh != normal) {
        result.push_back(fh);
      }
    }
    return result;
  }

  const FontHandle& GetFont(bool isBold, bool isItalic) const {
    if (isBold && isItalic) {
      return boldItalic;
    }
    if (isBold) {
      return bold;
    }
    if (isItalic) {
      return italic;
    }
    return normal;
  }
};

// list of fonts: primary font and fallback fonts
struct FontFamily {
  int linespace;
  float topLinespace;
  float dpiScale;

  std::vector<FontSet> fonts;
  ShapeDrawing shapeDrawing;

  TextureAtlas<false> textureAtlas;
  TextureAtlas<true> colorTextureAtlas;

  float defaultHeight;
  float defaultWidth;

  using FontFeatures = std::unordered_map<std::string, std::string>;
  FontFeatures fontFeatures;

  // Cache for characters that fallback to LastResort (unknown characters)
  std::unordered_set<std::string> lastResortCache;
  std::optional<FontHandle> lastResortFont;
  bool lastResortFontLoadFailed = false;

  static std::expected<FontFamily, std::runtime_error>
  Default(int linespace, float dpiScale);
  static std::expected<FontFamily, std::runtime_error>
  FromGuifont(std::string guifont, int linespace, float dpiScale);

  void TryChangeDpiScale(float dpiScale);
  void ChangeSize(float delta);
  void ResetSize();
  void UpdateLinespace(int linespace);
  void SetFontFeatures(const FontFeatures& fontFeatures);

  const Font& DefaultFont() const;
  glm::vec2 GetCharSize() const;
  float GetAscender() const;

  struct ResolvedFont {
    enum class Kind { Skip, ShapeDrawing, Regular };
    Kind kind;
    FontHandle font; // only valid when kind == Regular
  };

  ResolvedFont ResolveFont(const std::string& text, bool bold, bool italic);

  std::vector<ShapedGlyph> ShapeText(const std::string& text, const FontHandle& font);
  const GlyphInfo* GetGlyphInfo(const std::string& text); // box drawing, no font
  const GlyphInfo* GetGlyphInfo(UnderlineType underlineType);
  const GlyphInfo* GetGlyphInfo(StrikethroughTag);

  void ResetTextureAtlas(TextureResizeError error);

private:
  void UpdateFonts(std::function<FontHandle(const FontHandle&)> createFont);
  void ApplyFeaturesToFontSet(const FontSet& fontSet);
};

inline const Font& FontFamily::DefaultFont() const {
  return *fonts.front().normal;
}

// takes in account linespace
inline glm::vec2 FontFamily::GetCharSize() const {
  return {DefaultFont().charSize.x, DefaultFont().charSize.y + linespace};
}

// takes in account linespace
inline float FontFamily::GetAscender() const {
  return DefaultFont().ascender + topLinespace;
}
