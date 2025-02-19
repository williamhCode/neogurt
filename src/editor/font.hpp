#pragma once

#include "gfx/font_rendering/texture_atlas.hpp"
#include "gfx/font_rendering/font.hpp"
#include "gfx/font_rendering/shape_drawing.hpp"

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
  std::vector<FontSet> fonts;
  ShapeDrawing shapeDrawing;
  TextureAtlas textureAtlas;
  float defaultHeight;
  float defaultWidth;

  static FontFamily Default(float linespace, float dpiScale);
  static std::expected<FontFamily, std::string>
  FromGuifont(std::string guifont, float linespace, float dpiScale);

  bool TryChangeDpiScale(float dpiScale); // returns true if changed
  void ChangeSize(float delta);
  void ResetSize();

  const Font& DefaultFont() const;
  const GlyphInfo& GetGlyphInfo(char32_t charcode, bool bold, bool italic);
};
