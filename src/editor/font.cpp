#include "font.hpp"
#include "utils/expected.hpp"
#include "utils/logger.hpp"
#include <ranges>
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

  FontFamily fontFamily{
    .textureAtlas{(uint)size, dpiScale},
  };

  for (auto fontName : SplitStr(fontsStr, ',')) {
    FontSet fontSet;
    auto makeFontHandle =
      [&](bool bold, bool italic) -> std::expected<FontHandle, std::string> {
      return Font::FromName(
               {
                 .name = std::string(fontName),
                 .size = static_cast<int>(size),
                 .width = static_cast<int>(width),
                 .bold = bold,
                 .italic = italic,
               },
               dpiScale
      )
        .transform([&](auto&& font) {
          // check if the normal font can be reused
          if (fontSet.normal && font.path == fontSet.normal->path) {
            return fontSet.normal; // reuse the existing FontHandle
          }
          // otherwise, create a new FontHandle
          return std::make_shared<Font>(std::move(font));
        });
    };

    auto normalFont = makeFontHandle(bold, italic);
    if (!normalFont) return MakeUnexpected(normalFont);
    fontSet.normal = std::move(*normalFont);

    // If bold or italic is not set for font, try to create them
    if (!(bold || italic)) {
      auto boldFont = makeFontHandle(true, false);
      if (!boldFont) return MakeUnexpected(boldFont);
      fontSet.bold = std::move(*boldFont);

      auto italicFont = makeFontHandle(false, true);
      if (!italicFont) return MakeUnexpected(italicFont);
      fontSet.italic = std::move(*italicFont);

      auto boldItalicFont = makeFontHandle(true, true);
      if (!boldItalicFont) return MakeUnexpected(boldItalicFont);
      fontSet.boldItalic = std::move(*boldItalicFont);
    }

    fontFamily.fonts.push_back(std::move(fontSet));
  }

  return fontFamily;
}

const Font& FontFamily::DefaultFont() const {
  return *fonts.front().normal;
}

const Font::GlyphInfo&
FontFamily::GetGlyphInfo(uint32_t codepoint, bool bold, bool italic) {
  for (const auto& fontSet : fonts) {
    const auto& font = [&] {
      if (bold && italic) {
        return fontSet.boldItalic ? fontSet.boldItalic : fontSet.normal;
      }
      if (bold) {
        return fontSet.bold ? fontSet.bold : fontSet.normal;
      }
      if (italic) {
        return fontSet.italic ? fontSet.italic : fontSet.normal;
      }
      return fontSet.normal;
    }();

    if (const auto* glyphInfo = font->GetGlyphInfo(codepoint, textureAtlas)) {
      return *glyphInfo;
    }
  }

  const auto *glyphInfo = fonts.front().normal->GetGlyphInfo(' ', textureAtlas);
  if (glyphInfo == nullptr) {
    LOG_ERR("Failed to get glyph info for codepoint: {}", codepoint);
  }
  return *glyphInfo;
}
