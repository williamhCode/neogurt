#pragma once

#include "editor/highlight.hpp"
#include "gfx/font_rendering/texture_atlas.hpp"
#include "gfx/font_rendering/font.hpp"
#include "gfx/font_rendering/shape_drawing.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <expected>
#include <unordered_set>
#include <optional>

using FontHandle = std::shared_ptr<Font>;
// Font is shared with normal if bold/italic/boldItalic is not available
// If variation exists, it owns its own Font
struct FontSet {
  FontHandle normal;
  FontHandle bold;
  FontHandle italic;
  FontHandle boldItalic;
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

  const Font& DefaultFont() const;
  glm::vec2 GetCharSize() const;
  float GetAscender() const;

  const GlyphInfo* GetGlyphInfo(const std::string& text, bool bold, bool italic);
  const GlyphInfo* GetGlyphInfo(UnderlineType underlineType);

  void ResetTextureAtlas(TextureResizeError error);

private:
  void UpdateFonts(std::function<FontHandle(const FontHandle&)> createFont);
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
