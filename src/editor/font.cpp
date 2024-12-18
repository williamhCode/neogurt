#include "./font.hpp"
#include <ranges>
#include <string>
#include <boost/lexical_cast.hpp>

static auto SplitStr(std::string_view str, char delim) {
  return std::views::split(str, delim) |
         std::views::transform([](auto&& r) { return std::string_view(r); });
}

FontFamily FontFamily::Default(float linespace, float dpiScale) {
  return FromGuifont("SF Mono:h15", linespace, dpiScale).value();
}

std::expected<FontFamily, std::string>
FontFamily::FromGuifont(std::string guifont, float linespace, float dpiScale) {
  if (guifont.empty()) {
    return std::unexpected("Empty guifont");
  }

  std::ranges::replace(guifont, '_', ' ');

  // split string by colon, first string is required, options args can be any length
  std::string_view fontsStr;
  float height = 12;
  float width = 0;
  bool bold = false;
  bool italic = false;

  for (auto token : SplitStr(guifont, ':')) {
    if (token.empty()) continue;

    if (fontsStr.empty()) {
      fontsStr = token;
    } else {
      using namespace boost::conversion;
      if (token.starts_with("h")) {
        if (!try_lexical_convert(token.substr(1), height))
          return std::unexpected("Invalid guifont height: " + std::string(token));
      } else if (token.starts_with("w")) {
        if (!try_lexical_convert(token.substr(1), width))
          return std::unexpected("Invalid guifont width: " + std::string(token));
      } else if (token == "b") {
        bold = true;
      } else if (token == "i") {
        italic = true;
      }
    }
  }

  try {
    FontFamily fontFamily{
      .fonts = SplitStr(fontsStr, ',') | std::views::transform([&](auto&& fontName) {
        FontSet fontSet;
        auto makeFontHandle = [&](bool bold, bool italic) {
          auto font = Font::FromName(
            {
              .name = std::string(fontName),
              .height = height,
              .width = width,
              .bold = bold,
              .italic = italic,
            },
            linespace,
            dpiScale
          )
          .value(); // allow exception to propagate

          // reuse the existing FontHandle
          if (fontSet.normal && font.path == fontSet.normal->path) {
            return fontSet.normal;
          }
          // otherwise, create a new FontHandle
          return std::make_shared<Font>(std::move(font));
        };

        fontSet.normal = makeFontHandle(bold, italic);

        if (!(bold || italic)) {
          fontSet.bold = makeFontHandle(true, false);
          fontSet.italic = makeFontHandle(false, true);
          fontSet.boldItalic = makeFontHandle(true, true);
        }

        return fontSet;
      }) |
      std::ranges::to<std::vector>(),
      .shapeDrawing{fontFamily.DefaultFont().charSize, dpiScale},
      .textureAtlas{height, dpiScale},
      .defaultHeight = height,
      .defaultWidth = width,
    };
    return fontFamily;

  } catch (const std::bad_expected_access<std::string>& e) {
    return std::unexpected(e.error());
  }
}

bool FontFamily::TryChangeDpiScale(float dpiScale) {
  if (DefaultFont().dpiScale == dpiScale) return false;

  std::vector<FontSet> newFonts;
  for (auto& fontSet : fonts) {
    FontSet& newFontSet = newFonts.emplace_back();
    auto makeFontHandle = [&](const FontHandle& fontHandle) -> FontHandle {
      // check if the normal font can be reused
      if (newFontSet.normal && fontHandle->path == newFontSet.normal->path) {
        return newFontSet.normal;
      }
      return std::make_shared<Font>(
        fontHandle->path, fontHandle->height, fontHandle->width, fontHandle->linespace,
        dpiScale
      );
    };

    newFontSet.normal = makeFontHandle(fontSet.normal);
    newFontSet.bold = makeFontHandle(fontSet.bold);
    newFontSet.italic = makeFontHandle(fontSet.italic);
    newFontSet.boldItalic = makeFontHandle(fontSet.boldItalic);
  }
  fonts = std::move(newFonts);
  shapeDrawing = ShapeDrawing(DefaultFont().charSize, dpiScale);
  textureAtlas = TextureAtlas(DefaultFont().height, dpiScale);

  return true;
}

void FontFamily::ChangeSize(float delta) {
  std::vector<FontSet> newFonts;
  for (auto& fontSet : fonts) {
    FontSet& newFontSet = newFonts.emplace_back();

    auto makeFontHandle = [&](const FontHandle& fontHandle) -> FontHandle {
      // check if the normal font can be reused
      if (newFontSet.normal && fontHandle->path == newFontSet.normal->path) {
        return newFontSet.normal;
      }

      float newHeight = fontHandle->height + delta;
      newHeight = std::max(4.0f, newHeight);

      float widthHeightRatio = defaultWidth / defaultHeight;
      float newWidth = newHeight * widthHeightRatio;

      return std::make_shared<Font>(
        fontHandle->path, newHeight, newWidth, fontHandle->linespace,
        fontHandle->dpiScale
      );
    };

    newFontSet.normal = makeFontHandle(fontSet.normal);
    newFontSet.bold = makeFontHandle(fontSet.bold);
    newFontSet.italic = makeFontHandle(fontSet.italic);
    newFontSet.boldItalic = makeFontHandle(fontSet.boldItalic);
  }
  fonts = std::move(newFonts);
  shapeDrawing = ShapeDrawing(DefaultFont().charSize, DefaultFont().dpiScale);
  textureAtlas = TextureAtlas(DefaultFont().height, DefaultFont().dpiScale);
}

void FontFamily::ResetSize() {
  std::vector<FontSet> newFonts;
  for (auto& fontSet : fonts) {
    FontSet& newFontSet = newFonts.emplace_back();
    auto makeFontHandle = [&](const FontHandle& fontHandle) -> FontHandle {
      // check if the normal font can be reused
      if (newFontSet.normal && fontHandle->path == newFontSet.normal->path) {
        return newFontSet.normal;
      }
      return std::make_shared<Font>(
        fontHandle->path, defaultHeight, defaultWidth, fontHandle->linespace,
        fontHandle->dpiScale
      );
    };

    newFontSet.normal = makeFontHandle(fontSet.normal);
    newFontSet.bold = makeFontHandle(fontSet.bold);
    newFontSet.italic = makeFontHandle(fontSet.italic);
    newFontSet.boldItalic = makeFontHandle(fontSet.boldItalic);
  }
  fonts = std::move(newFonts);
  shapeDrawing = ShapeDrawing(DefaultFont().charSize, DefaultFont().dpiScale);
  textureAtlas = TextureAtlas(DefaultFont().height, DefaultFont().dpiScale);
}

const Font& FontFamily::DefaultFont() const {
  return *fonts.front().normal;
}

const GlyphInfo&
FontFamily::GetGlyphInfo(char32_t charcode, bool bold, bool italic) {
  // box drawing chars
  if (charcode >= 0x2500 && charcode <= 0x259F) {
    if (const auto *glyphInfo = shapeDrawing.GetGlyphInfo(charcode, textureAtlas)) {
      return *glyphInfo;
    }
  }

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

    if (const auto* glyphInfo = font->GetGlyphInfo(charcode, textureAtlas)) {
      return *glyphInfo;
    }
  }

  // TODO: draw a question mark symbol instead
  for (const auto& fontSet : fonts) {
    if (const auto* glyphInfo = fontSet.normal->GetGlyphInfo(' ', textureAtlas)) {
      return *glyphInfo;
    }
  }
  throw std::runtime_error("Failed to get glyph for space character");
}
