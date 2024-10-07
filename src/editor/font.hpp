#pragma once

#include "gfx/font.hpp"
#include "gfx/box_drawing.hpp"
#include "gfx/texture_atlas.hpp"

#include <array>
#include <string_view>
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
  BoxDrawing boxDrawing;
  TextureAtlas textureAtlas;
  float defaultHeight;
  float defaultWidth;

  static std::expected<FontFamily, std::string>
  FromGuifont(std::string guifont, float linespace, float dpiScale);
  // static FontFamily Default(float dpiScale);

  void ChangeDpiScale(float dpiScale);
  void ChangeSize(float delta);
  void ResetSize();

  const Font& DefaultFont() const;
  const GlyphInfo& GetGlyphInfo(char32_t charcode, bool bold, bool italic);
};
