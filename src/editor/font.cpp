#include "font.hpp"
#include "gfx/font_handle.hpp"
#include <optional>
#include <ranges>
#include <sstream>
#include <string>

static auto SplitStr(std::string_view str, char delim) {
  return std::views::split(str, delim) |
         std::views::transform([](auto r) { return std::string_view(r); });
}

std::expected<FontFamily, std::string>
FontFamily::FromGuifont(std::string_view guifont, float dpiScale) {
  if (guifont.empty()) {
    return std::unexpected("Empty guifont");
  }

  // split string by colon, first string is required, options args can be any length
  std::string_view fontsStr;
  float size = 12;
  float width = 0;
  bool bold = false;
  bool italic = false;

  for (auto token : SplitStr(guifont, ':')) {
    if (token.empty()) continue;

    if (fontsStr.empty()) {
      fontsStr = token;
    } else {
      if (token.starts_with("h")) {
        try {
          size = std::stof(std::string(token.substr(1)));
        } catch (const std::invalid_argument& e) {
          return std::unexpected("Invalid guifont height: " + std::string(token));
        }
      } else if (token.starts_with("w")) {
        try {
          width = std::stof(std::string(token.substr(1)));
        } catch (const std::invalid_argument& e) {
          return std::unexpected("Invalid guifont width: " + std::string(token));
        }
      } else if (token == "b") {
        bold = true;
      } else if (token == "i") {
        italic = true;
      }
    }
  }

  FontFamily fontFamily;
  for (auto font : SplitStr(fontsStr, ',')) {
    auto makeFont = [&](bool bold, bool italic) {
      return FontHandleFromName(
        {
          .name = std::string(font),
          .size = (int)size,
          .width = (int)width,
          .bold = bold,
          .italic = italic,
        },
        dpiScale
      );
    };

    FontSet fontSet;

    auto normalFont = makeFont(bold, italic);
    if (!normalFont) {
      return std::unexpected(normalFont.error());
    }
    fontSet.normal = std::move(*normalFont);

    // If bold or italic is not set for font, try to create them
    if (!(bold || italic)) {
      if (auto boldFont = makeFont(true, false)) {
        fontSet.bold = std::move(*boldFont);
      }
      if (auto italicFont = makeFont(false, true)) {
        fontSet.italic = std::move(*italicFont);
      }
      if (auto boldItalicFont = makeFont(true, true)) {
        fontSet.boldItalic = std::move(*boldItalicFont);
      }
    }

    fontFamily.fonts.push_back(std::move(fontSet));
  }

  return fontFamily;
}
