#include "./font.hpp"
#include "gfx/font_rendering/font_locator.hpp"
#include "utils/logger.hpp"
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

            // reuse the existing FontHandle, if font is same as normal font
            if (fontSet.normal && font.path == fontSet.normal->path) {
              return fontSet.normal;
            }
            // otherwise, create a new FontHandle
            return std::make_shared<Font>(std::move(font));
          };

          fontSet.normal = makeFontHandle(bold, italic);

          // if bold or italic, entire fontset is the same
          if (bold || italic) {
            fontSet.bold = fontSet.normal;
            fontSet.italic = fontSet.normal;
            fontSet.boldItalic = fontSet.normal;

          } else {
            fontSet.bold = makeFontHandle(true, false);
            fontSet.italic = makeFontHandle(false, true);
            fontSet.boldItalic = makeFontHandle(true, true);
          }

          return fontSet;
        }) |
        std::ranges::to<std::vector>(),
      .shapeDrawing = ShapeDrawing(
        fontFamily.GetCharSize(), fontFamily.DefaultFont().underlineThickness,
        fontFamily.DefaultFont().strikeoutThickness, dpiScale
      ),

      .textureAtlas =
        TextureAtlas<false>(fontFamily.DefaultFont().charSize.y, dpiScale),
      .colorTextureAtlas =
        TextureAtlas<true>(fontFamily.DefaultFont().charSize.y, dpiScale),

      .defaultHeight = height,
      .defaultWidth = width,
    };
    return fontFamily;

  } catch (std::bad_expected_access<std::runtime_error>& e) {
    return std::unexpected(std::move(e.error()));
  }
}

void FontFamily::TryChangeDpiScale(float _dpiScale) {
  if (dpiScale == _dpiScale) return;
  dpiScale = _dpiScale;

  UpdateFonts([&](const FontHandle& fontHandle) -> FontHandle {
    return std::make_shared<Font>(
      fontHandle->path, fontHandle->height, fontHandle->width, dpiScale
    );
  });
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
      // resuse new normal font, if font handle is same as old normal font
      if (newFontSet.normal && fontHandle == fontSet.normal) {
        return newFontSet.normal;
      }
      return createFont(fontHandle);
    };

    newFontSet.normal = makeFontHandle(fontSet.normal);
    newFontSet.bold = makeFontHandle(fontSet.bold);
    newFontSet.italic = makeFontHandle(fontSet.italic);
    newFontSet.boldItalic = makeFontHandle(fontSet.boldItalic);

    ApplyFeaturesToFontSet(newFontSet);
  }

  fonts = std::move(newFonts);
  shapeDrawing = ShapeDrawing(
    GetCharSize(), DefaultFont().underlineThickness, DefaultFont().strikeoutThickness,
    dpiScale
  );
  textureAtlas = TextureAtlas<false>(DefaultFont().charSize.y, dpiScale);
  colorTextureAtlas = TextureAtlas<true>(DefaultFont().charSize.y, dpiScale);

  if (lastResortFont.has_value()) {
    lastResortFont = createFont(*lastResortFont);
  }
}

void FontFamily::UpdateLinespace(int _linespace) {
  // NOTE: updates to new box drawing chars, but old ones stay in the atlas
  linespace = _linespace;
  topLinespace = RoundToPixel(linespace / 2.0, dpiScale);
  shapeDrawing = ShapeDrawing(
    GetCharSize(), DefaultFont().underlineThickness, DefaultFont().strikeoutThickness,
    dpiScale
  );
}

void FontFamily::SetFontFeatures(const FontFeatures& _fontFeatures) {
  fontFeatures = _fontFeatures;

  for (auto& fontSet : fonts) {
    ApplyFeaturesToFontSet(fontSet);
  }
}

void FontFamily::ApplyFeaturesToFontSet(const FontSet& fontSet) {
  for (const FontHandle* fontPtr : fontSet.UniqueFonts()) {
    const auto& font = *fontPtr;

    auto it = fontFeatures.find(font->familyName);
    if (it != fontFeatures.end()) {
      font->SetFeatures(it->second);
      continue;
    }

    auto fallback = fontFeatures.find("");
    if (fallback != fontFeatures.end()) {
      font->SetFeatures(fallback->second);
      continue;
    }

    font->SetFeatures("");
  }
}

FontFamily::ResolvedFont FontFamily::ResolveFont(const std::string& text, bool bold, bool italic) {
  using Kind = ResolvedFont::Kind;

  // optimize for empty text / space
  if (text.empty() || text == " ") return {Kind::Skip};

  // box drawing / undercurl shapes — rendered by ShapeDrawing, not a font
  if (ShapeDrawing::ShouldRenderText(text)) return {Kind::ShapeDrawing};

  // Check if this text is in the LastResort cache (unknown character)
  if (lastResortCache.contains(text)) {
    if (lastResortFontLoadFailed) return {Kind::Skip};

    if (lastResortFont.has_value() && (*lastResortFont)->ShouldRenderText(text)) {
      return {Kind::Regular, *lastResortFont};
    }
    return {Kind::Skip};
  }

  // Try to find glyph in existing fonts
  for (const auto& fontSet : fonts) {
    const auto& font = fontSet.GetFont(bold, italic);
    if (font->ShouldRenderText(text)) return {Kind::Regular, font};
  }

  // Try to find a fallback font
  std::string fallbackFontName = FindFallbackFontForCharacter(text);

  // If no font found or LastResort, cache it and load LastResort font
  if (fallbackFontName.empty() ||
      fallbackFontName == ".LastResort" ||
      fallbackFontName == "LastResort") {

    lastResortCache.insert(text);

    if (!lastResortFont.has_value() && !lastResortFontLoadFailed) {
      auto font = Font::FromName(
        {
          .name = "LastResort",
          .height = DefaultFont().height,
          .width = DefaultFont().width,
          .bold = false,
          .italic = false,
        },
        dpiScale
      );

      if (font.has_value()) {
        lastResortFont = std::make_shared<Font>(std::move(*font));
      } else {
        lastResortFontLoadFailed = true;
        return {Kind::Skip};
      }
    }

    if (lastResortFont.has_value() && (*lastResortFont)->ShouldRenderText(text)) {
      return {Kind::Regular, *lastResortFont};
    }
    return {Kind::Skip};
  }

  // Valid fallback font found - add to fonts vector
  try {
    FontSet fallbackSet;

    auto makeFontHandle = [&](bool bold, bool italic) {
      auto font = Font::FromName(
        {
          .name = fallbackFontName,
          .height = DefaultFont().height,
          .width = DefaultFont().width,
          .bold = bold,
          .italic = italic,
        },
        dpiScale
      )
      .value();

      if (fallbackSet.normal && font.path == fallbackSet.normal->path) {
        return fallbackSet.normal;
      }
      return std::make_shared<Font>(std::move(font));
    };

    fallbackSet.normal = makeFontHandle(false, false);
    fallbackSet.bold = makeFontHandle(true, false);
    fallbackSet.italic = makeFontHandle(false, true);
    fallbackSet.boldItalic = makeFontHandle(true, true);

    ApplyFeaturesToFontSet(fallbackSet);

    fonts.push_back(std::move(fallbackSet));

    const auto& font = fonts.back().GetFont(bold, italic);
    if (font->ShouldRenderText(text)) return {Kind::Regular, font};

  } catch (std::bad_expected_access<std::runtime_error>&) {
    LOG_ERR("Failed to load fallback font: {}", fallbackFontName);
  }

  return {Kind::Skip};
}

std::vector<ShapedGlyph> FontFamily::ShapeText(const std::string& text, const FontHandle& font) {
  return font->ShapeText(text, textureAtlas, colorTextureAtlas);
}

const GlyphInfo* FontFamily::GetGlyphInfo(const std::string& text) {
  return shapeDrawing.GetGlyphInfo(text, textureAtlas);
}

const GlyphInfo* FontFamily::GetGlyphInfo(UnderlineType underlineType) {
  return shapeDrawing.GetGlyphInfo(underlineType, textureAtlas);
}

const GlyphInfo* FontFamily::GetGlyphInfo(StrikethroughTag tag) {
  return shapeDrawing.GetGlyphInfo(tag, textureAtlas);
}

void FontFamily::ResetTextureAtlas(TextureResizeError error) {
  switch (error) {
    case TextureResizeError::Normal:
      textureAtlas = TextureAtlas<false>(DefaultFont().charSize.y, dpiScale);
      // reset all glyph info maps that use the normal texture atlas
      for (auto& fontSet : fonts) {
        fontSet.normal->glyphInfoMap = {};
        fontSet.bold->glyphInfoMap = {};
        fontSet.italic->glyphInfoMap = {};
        fontSet.boldItalic->glyphInfoMap = {};
      }
      if (lastResortFont.has_value()) {
        (*lastResortFont)->glyphInfoMap = {};
      }
      shapeDrawing.glyphInfoMap = {};
      shapeDrawing.underlineGlyphInfoMap = {};
      shapeDrawing.strikethroughGlyphInfo = {};
      break;

    case TextureResizeError::Colored:
      colorTextureAtlas = TextureAtlas<true>(DefaultFont().charSize.y, dpiScale);
      // reset all glyph info maps that use the colored texture atlas
      for (auto& fontSet : fonts) {
        fontSet.normal->emojiGlyphInfoMap = {};
        fontSet.bold->emojiGlyphInfoMap = {};
        fontSet.italic->emojiGlyphInfoMap = {};
        fontSet.boldItalic->emojiGlyphInfoMap = {};
      }
      if (lastResortFont.has_value()) {
        (*lastResortFont)->emojiGlyphInfoMap = {};
      }
      break;
  }
}
