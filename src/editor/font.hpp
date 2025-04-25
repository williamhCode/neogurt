#pragma once

#include "gfx/font_rendering/texture_atlas.hpp"
#include "gfx/font_rendering/font.hpp"
#include "gfx/font_rendering/shape_drawing.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <expected>

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

  static std::expected<FontFamily, std::runtime_error>
  Default(int linespace, float dpiScale);
  static std::expected<FontFamily, std::runtime_error>
  FromGuifont(std::string guifont, int linespace, float dpiScale);

  bool TryChangeDpiScale(float dpiScale); // returns true if changed
  void ChangeSize(float delta);
  void ResetSize();
  void UpdateLinespace(int linespace);

  const Font& DefaultFont() const;
  glm::vec2 GetCharSize() const;
  float GetAscender() const;

  const GlyphInfo& GetGlyphInfo(const std::string& text, bool bold, bool italic);

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
