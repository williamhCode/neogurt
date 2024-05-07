#pragma once
#include "gfx/font_handle.hpp"
#include <string_view>
#include <vector>
#include <optional>
#include <expected>

// Normal is guarenteed to be present.
// If style is set explicitly, only normal style will be present,
// and normal member will be set to that style.
struct FontSet {
  FontHandle normal;
  FontHandle bold; // optional
  FontHandle italic; // optional
  FontHandle boldItalic; // optional
};

// list of fonts: primary font and fallback fonts
struct FontFamily {
  std::vector<FontSet> fonts;

  static std::expected<FontFamily, std::string>
  FromGuifont(std::string_view guifont, float dpiScale);

  // static FontFamily Default(float dpiScale);
};

