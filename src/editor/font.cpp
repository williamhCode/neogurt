#include "./font.hpp"
#include <ranges>
#include <string>
#include <algorithm>
#include <boost/lexical_cast.hpp>

static float ClampHeight(float height) {
  return std::clamp(height, 4.0f, 72.0f);
}

static auto SplitStr(std::string_view str, std::string_view delim) {
  return std::views::split(str, delim) |
         std::views::transform([](auto&& r) { return std::string_view(r); });
}

std::expected<FontFamily, std::runtime_error>
FontFamily::Default(int linespace, float dpiScale) {
  return FromGuifont("SF Mono:h15", linespace, dpiScale);
}

std::expected<FontFamily, std::runtime_error>
FontFamily::FromGuifont(std::string guifont, int linespace, float dpiScale) {
  if (guifont.empty()) {
    return std::unexpected(std::runtime_error("Empty guifont"));
  }

  std::ranges::replace(guifont, '_', ' ');

  // split string by colon, first string is required, options args can be any length
  std::string_view fontsStr;
  float height = 12;
  float width = 0;
  bool bold = false;
  bool italic = false;

  for (std::string_view token : SplitStr(guifont, ":")) {
    if (token.empty()) continue;

    if (fontsStr.empty()) {
      fontsStr = token;
    } else {
      using namespace boost::conversion;
      if (token.starts_with("h")) {
        if (!try_lexical_convert(token.substr(1), height))
          return std::unexpected(
            std::runtime_error("Invalid guifont height: " + std::string(token))
          );
      } else if (token.starts_with("w")) {
        if (!try_lexical_convert(token.substr(1), width))
          return std::unexpected(
            std::runtime_error("Invalid guifont width: " + std::string(token))
          );
      } else if (token == "b") {
        bold = true;
      } else if (token == "i") {
        italic = true;
      }
    }
  }

  try {
    FontFamily fontFamily{
      .linespace = linespace,
      .topLinespace = RoundToPixel(linespace / 2.0, dpiScale),
      .dpiScale = dpiScale,
      .fonts =
        SplitStr(fontsStr, ",") | std::views::transform([&](std::string_view fontName) {
          FontSet fontSet;
          auto makeFontHandle = [&](bool bold, bool italic) {
            auto font = Font::FromName(
              {
                .name = std::string(fontName),
                .height = ClampHeight(height),
                .width = width,
                .bold = bold,
                .italic = italic,
              },
              dpiScale
            )
            .value(); // allow exception to propagate
            // (yes we're using exceptions for control flow but this code doesn't needa run super fast)

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
      .shapeDrawing{fontFamily.GetCharSize(), dpiScale},
      .textureAtlas{fontFamily.DefaultFont().charSize.y, dpiScale},
      .colorTextureAtlas{fontFamily.DefaultFont().charSize.y, dpiScale},
      .defaultHeight = height,
      .defaultWidth = width,
    };
    return fontFamily;

  } catch (std::bad_expected_access<std::runtime_error>& e) {
    return std::unexpected(std::move(e.error()));
  }
}

bool FontFamily::TryChangeDpiScale(float _dpiScale) {
  if (dpiScale == _dpiScale) return false;

  UpdateFonts([&](const FontHandle& fontHandle) -> FontHandle {
    return std::make_shared<Font>(
      fontHandle->path, fontHandle->height, fontHandle->width, dpiScale
    );
  });

  return true;
}

void FontFamily::ChangeSize(float delta) {
  UpdateFonts([&](const FontHandle& fontHandle) -> FontHandle{
    float newHeight = fontHandle->height + delta;
    newHeight = ClampHeight(newHeight);

    float widthHeightRatio = defaultWidth / defaultHeight;
    float newWidth = newHeight * widthHeightRatio;

    return std::make_shared<Font>(fontHandle->path, newHeight, newWidth, dpiScale);
  });
}

void FontFamily::ResetSize() {
  UpdateFonts([&](const FontHandle& fontHandle) -> FontHandle {
    return std::make_shared<Font>(
      fontHandle->path, defaultHeight, defaultWidth, dpiScale
    );
  });
}

void FontFamily::UpdateFonts(std::function<FontHandle(const FontHandle&)> createFont) {
  std::vector<FontSet> newFonts;
  for (const auto& fontSet : fonts) {
    FontSet& newFontSet = newFonts.emplace_back();

    auto makeFontHandle = [&](const FontHandle& fontHandle) -> FontHandle {
      // check if the normal font can be reused
      if (newFontSet.normal && fontHandle->path == newFontSet.normal->path) {
        return newFontSet.normal;
      }
      return createFont(fontHandle);
    };

    newFontSet.normal = makeFontHandle(fontSet.normal);
    newFontSet.bold = makeFontHandle(fontSet.bold);
    newFontSet.italic = makeFontHandle(fontSet.italic);
    newFontSet.boldItalic = makeFontHandle(fontSet.boldItalic);
  }

  fonts = std::move(newFonts);
  shapeDrawing = ShapeDrawing(GetCharSize(), dpiScale);
  textureAtlas = TextureAtlas<false>(DefaultFont().charSize.y, dpiScale);
  colorTextureAtlas = TextureAtlas<true>(DefaultFont().charSize.y, dpiScale);
}

void FontFamily::UpdateLinespace(int _linespace) {
  linespace = _linespace;
  RoundToPixel(linespace / 2.0, dpiScale);
  shapeDrawing = ShapeDrawing(GetCharSize(), dpiScale);
}

const GlyphInfo&
FontFamily::GetGlyphInfo(const std::string& text, bool bold, bool italic) {
  // shapes
  if (const auto *glyphInfo = shapeDrawing.GetGlyphInfo(text, textureAtlas)) {
    return *glyphInfo;
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

    if (const auto* glyphInfo =
          font->GetGlyphInfo(text, textureAtlas, colorTextureAtlas)) {
      return *glyphInfo;
    }
  }

  // TODO: draw a question mark symbol instead
  for (const auto& fontSet : fonts) {
    if (const auto* glyphInfo =
          fontSet.normal->GetGlyphInfo(" ", textureAtlas, colorTextureAtlas)) {
      return *glyphInfo;
    }
  }
  throw std::runtime_error("Failed to get glyph for space character");
}
