#include "./font_coretext.hpp"
#include "./font_locator.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "utils/unicode.hpp"
#include <cmath>
#include <mdspan>
#include <ranges>
#include <span>
#include <unordered_set>

#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>

// Check if a character is in the "text by default" range
// These characters are rendered as text unless followed by U+FE0F
// Data source: docs/emoji/emoji-test.txt
static bool IsTextByDefault(char32_t c) {
  static const std::unordered_set<char32_t> emojiByDefault = {
    0x231A, // ⌚ watch
    0x231B, // ⌛ hourglass done
    0x23E9, // ⏩ fast-forward button
    0x23EA, // ⏪ fast reverse button
    0x23EB, // ⏫ fast up button
    0x23EC, // ⏬ fast down button
    0x23F0, // ⏰ alarm clock
    0x23F3, // ⏳ hourglass not done
    0x25FD, // ◽ white medium-small square
    0x25FE, // ◾ black medium-small square
    0x2614, // ☔ umbrella with rain drops
    0x2615, // ☕ hot beverage
    0x2648, 0x2649, 0x264A, 0x264B, 0x264C, 0x264D, // ♈-♍ zodiac
    0x264E, 0x264F, 0x2650, 0x2651, 0x2652, 0x2653, // ♎-♓ zodiac
    0x267F, // ♿ wheelchair symbol
    0x2693, // ⚓ anchor
    0x26A1, // ⚡ high voltage
    0x26AA, // ⚪ white circle
    0x26AB, // ⚫ black circle
    0x26BD, // ⚽ soccer ball
    0x26BE, // ⚾ baseball
    0x26C4, // ⛄ snowman without snow
    0x26C5, // ⛅ sun behind cloud
    0x26CE, // ⛎ ophiuchus
    0x26D4, // ⛔ no entry
    0x26EA, // ⛪ church
    0x26F2, // ⛲ fountain
    0x26F3, // ⛳ flag in hole
    0x26F5, // ⛵ sailboat
    0x26FA, // ⛺ tent
    0x26FD, // ⛽ fuel pump
    0x2705, // ✅ check mark button
    0x270A, // ✊ raised fist
    0x270B, // ✋ raised hand
    0x2728, // ✨ sparkles
    0x274C, // ❌ cross mark
    0x274E, // ❎ cross mark button
    0x2753, // ❓ red question mark
    0x2754, // ❔ white question mark
    0x2755, // ❕ white exclamation mark
    0x2757, // ❗ red exclamation mark
    0x2795, // ➕ plus
    0x2796, // ➖ minus
    0x2797, // ➗ divide
    0x27B0, // ➰ curly loop
    0x27BF, // ➿ double curly loop
    0x2B1B, // ⬛ black large square
    0x2B1C, // ⬜ white large square
    0x2B50, // ⭐ star
    0x2B55, // ⭕ hollow red circle
  };

  bool isTextDefaultRange =
    (c >= 0x2000 && c <= 0x27BF) ||  // Misc symbols, dingbats, arrows
    (c >= 0x2900 && c <= 0x2BFF) ||  // Misc math symbols, arrows, geometric
    (c >= 0x3000 && c <= 0x303F) ||  // CJK symbols
    (c >= 0x3200 && c <= 0x32FF);    // Enclosed CJK

  return isTextDefaultRange && !emojiByDefault.contains(c);
}

// Check if a character should always be rendered as text (never emoji)
// These characters have emoji variants but they are non-colored as well
static bool AlwaysText(char32_t c) {
  static const std::unordered_set<char32_t> textOnly = {
    0x2640, // ♀ female sign
    0x2642, // ♂ male sign
    0x2695, // ⚕ medical symbol
  };
  return textOnly.contains(c);
}

// Check if a rendered glyph should be accepted based on emoji presentation rules
// Returns true if glyph should be accepted, false if it should be rejected
static bool ShouldAcceptGlyphForEmojiPresentation(
  const std::u32string& u32text,
  bool isColorEmoji
) {
  // Text-only characters should always render
  if (AlwaysText(u32text[0])) {
    return true;
  }

  // Check for variation selectors to determine if we should accept this glyph
  bool hasEmojiSelector = false;  // U+FE0F - emoji presentation
  for (char32_t c : u32text) {
    if (c == 0xFE0F) {
      hasEmojiSelector = true;
      break;
    }
  }

  // Check if character is in a "text-default but emoji-capable" range
  // These characters are TEXT by default and only become emoji with FE0F
  // If no FE0F is present, treat them as having FE0E (text selector)
  bool hasTextSelector = false;   // U+FE0E - text presentation
  if (!hasEmojiSelector && u32text.size() == 1) {
    if (IsTextByDefault(u32text[0])) {
      hasTextSelector = true;
    }
  }

  // Text requested emoji presentation but font returned text glyph
  if (hasEmojiSelector && !isColorEmoji) {
    return false;
  }
  // Text requested text presentation but font returned emoji glyph
  if (hasTextSelector && isColorEmoji) {
    return false;
  }

  return true;
}

std::expected<Font, std::runtime_error>
Font::FromName(const FontDescriptorWithName& desc, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  if (fontPath.empty()) {
    return std::unexpected(std::runtime_error("Failed to find font for: " + desc.name));
  }
  try {
    Font font(fontPath, desc.height, desc.width, dpiScale);
    return font;
  } catch (std::runtime_error& e) {
    return std::unexpected(std::move(e));
  }
}

Font::Font(std::string _path, float _height, float _width, float _dpiScale)
    : path(std::move(_path)), familyName(GetFontFamilyName(path)), height(_height),
      width(_width), dpiScale(_dpiScale) {

  int trueHeight = height * dpiScale;
  height = trueHeight / dpiScale;

  int trueWidth = width * dpiScale;
  width = trueWidth / dpiScale;

  CFURLRef url = CFURLCreateFromFileSystemRepresentation(
    nullptr, reinterpret_cast<const UInt8*>(path.c_str()), path.size(), false
  );
  CFArrayRef descriptors = CTFontManagerCreateFontDescriptorsFromURL(url);
  CFRelease(url);
  auto descriptor = (CTFontDescriptorRef)CFArrayGetValueAtIndex(descriptors, 0);

  // Apply horizontal scaling if a custom width was requested
  CGAffineTransform transform = CGAffineTransformIdentity;
  if (trueWidth > 0 && trueHeight > 0 && trueWidth != trueHeight) {
    auto scaleX = (CGFloat)trueWidth / trueHeight;
    transform = CGAffineTransformMakeScale(scaleX, 1.0);
  }

  CTFontRef rawFont = CTFontCreateWithFontDescriptor(descriptor, trueHeight, &transform);
  CFRelease(descriptors);
  if (!rawFont) {
    throw std::runtime_error("Failed to create CTFont for: " + path);
  }
  ctFont = CTFontPtr(rawFont);

  hbFont = hb::unique_ptr(hb_coretext_font_create(ctFont.get()));
  hbBuffer = hb::unique_ptr(hb_buffer_create());

  // ── Metrics (all in physical pixels = ptSize units in 1:1 context) ──────────
  float ascentPx = CTFontGetAscent(ctFont.get());
  float descentPx = CTFontGetDescent(ctFont.get());
  float leadingPx = CTFontGetLeading(ctFont.get());

  // charSize.x: advance of 'M' (same heuristic as FreeType version)
  UniChar mChar = 'M';
  CGGlyph mGlyph;
  CTFontGetGlyphsForCharacters(ctFont.get(), &mChar, &mGlyph, 1);
  if (mGlyph != 0) {
    CGSize advance;
    CTFontGetAdvancesForGlyphs(
      ctFont.get(), kCTFontOrientationHorizontal, &mGlyph, &advance, 1
    );
    charSize.x = std::round(advance.width) / dpiScale;
  } else {
    charSize.x = std::round(height * dpiScale) / dpiScale;
  }
  charSize.y = std::round(ascentPx + descentPx + leadingPx) / dpiScale;
  ascender = (ascentPx + leadingPx / 2) / dpiScale;
  // ascender = ascentPx / dpiScale;

  underlinePosition = CTFontGetUnderlinePosition(ctFont.get()) / dpiScale;
  underlineThickness = CTFontGetUnderlineThickness(ctFont.get()) / dpiScale;

  strikeoutPosition = CTFontGetXHeight(ctFont.get()) * 0.5 / dpiScale;
  strikeoutThickness = underlineThickness;

  isColorFont = (CTFontGetSymbolicTraits(ctFont.get()) & kCTFontColorGlyphsTrait) != 0;
}

void Font::SetFeatures(std::string_view featuresStr) {
  features.clear();
  for (auto part : std::views::split(featuresStr, std::string_view{","})) {
    std::string_view token(part);
    size_t start = token.find_first_not_of(' ');
    if (start == std::string_view::npos) continue;
    token = token.substr(start, token.find_last_not_of(' ') - start + 1);
    hb_feature_t feature;
    if (hb_feature_from_string(token.data(), token.size(), &feature)) {
      features.push_back(feature);
    }
  }
}

bool Font::ShouldRenderText(const std::string& text) {
  if (supportedTexts.contains(text)) return true;

  std::u16string u16 = Utf8ToUtf16(text);
  std::u32string u32 = Utf8ToUtf32(text);
  CGGlyph glyph = 0;

  if (u32.size() == 1 && features.empty()) {
    // Fast path: direct glyph lookup
    if (!CTFontGetGlyphsForCharacters(
          ctFont.get(), reinterpret_cast<uint16_t*>(u16.data()), &glyph,
          (CFIndex)u16.size()
        ) || glyph == 0) {
      return false;
    }
  } else {
    // Shaped path: use HarfBuzz (handles multi-char sequences and features)
    hb_buffer_reset(hbBuffer);
    hb_buffer_add_utf8(hbBuffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hbBuffer);
    hb_shape(hbFont, hbBuffer, features.data(), features.size());

    uint len = hb_buffer_get_length(hbBuffer);
    if (len == 0) return false;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
    if (info[0].codepoint == 0) return false;
  }

  if (!ShouldAcceptGlyphForEmojiPresentation(u32, isColorFont)) return false;

  supportedTexts.insert(text);
  return true;
}

std::vector<ShapedGlyph> Font::ShapeText(
  const std::string& text,
  TextureAtlas<false>& textureAtlas,
  TextureAtlas<true>& colorTextureAtlas
) {
  std::u32string u32 = Utf8ToUtf32(text);

  hb_buffer_reset(hbBuffer);
  hb_buffer_add_utf32(
    hbBuffer, reinterpret_cast<const uint32_t*>(u32.data()), u32.size(), 0, -1
  );
  hb_buffer_guess_segment_properties(hbBuffer);
  hb_shape(hbFont, hbBuffer, features.data(), features.size());

  hb_glyph_info_t*     infos = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
  hb_glyph_position_t* pos   = hb_buffer_get_glyph_positions(hbBuffer, nullptr);
  uint len = hb_buffer_get_length(hbBuffer);

  std::vector<ShapedGlyph> result;
  for (uint i = 0; i < len; i++) {
    int clusterStart = infos[i].cluster;
    int clusterEnd   = (i + 1 < len) ? infos[i + 1].cluster : (int)u32.size();
    result.push_back({
      .glyphInfo = RasterizeGlyph(infos[i].codepoint, textureAtlas, colorTextureAtlas),
      .numCells  = clusterEnd - clusterStart,
      .xOffset   = pos[i].x_offset / 64.f / dpiScale,
    });
  }
  return result;
}

GlyphInfo* Font::RasterizeGlyph(
  uint32_t glyphIndex,
  TextureAtlas<false>& textureAtlas,
  TextureAtlas<true>& colorTextureAtlas
) {
  // Check caches
  if (auto it = glyphInfoMap.find(glyphIndex); it != glyphInfoMap.end()) {
    return &it->second;
  }
  if (auto it = emojiGlyphInfoMap.find(glyphIndex); it != emojiGlyphInfoMap.end()) {
    return &it->second;
  }

  CGGlyph glyph = glyphIndex;
  CGRect bounds = CTFontGetBoundingRectsForGlyphs(
    ctFont.get(), kCTFontOrientationHorizontal, &glyph, nullptr, 1
  );

  // Determine integer pixel boundaries relative to the baseline (0,0)
  // This snaps the bounding box to the nearest outer pixel edges.
  int xMin = std::floor(bounds.origin.x);
  int xMax = std::ceil(bounds.origin.x + bounds.size.width);
  int yMin = std::floor(bounds.origin.y);
  int yMax = std::ceil(bounds.origin.y + bounds.size.height);

  size_t bitmapWidth = xMax - xMin;
  size_t bitmapHeight = yMax - yMin;

  CGPoint drawOrigin = {(CGFloat)-xMin, (CGFloat)-yMin};

  GlyphInfo glyphInfo{
    .localPoss = MakeRegion(
      {
        xMin / dpiScale,
        -yMax / dpiScale,
      },
      {
        bitmapWidth / dpiScale,
        bitmapHeight / dpiScale,
      }
    ),
  };

  // emoji glyphs
  if (isColorFont) {
    // kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst = BGRA on little-endian
    std::vector<uint8_t> buf(bitmapWidth * bitmapHeight * 4, 0);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
      buf.data(), bitmapWidth, bitmapHeight, 8, bitmapWidth * 4, cs,
      kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst
    );
    CGColorSpaceRelease(cs);

    CTFontDrawGlyphs(ctFont.get(), &glyph, &drawOrigin, 1, context);
    CGContextRelease(context);

    std::extents shape{bitmapHeight, bitmapWidth};
    std::array strides{bitmapWidth, 1uz};
    auto view = std::mdspan(
      reinterpret_cast<uint32_t*>(buf.data()),
      std::layout_stride::mapping{shape, strides}
    );

    glyphInfo.atlasRegion = colorTextureAtlas.AddGlyph(view);
    glyphInfo.isEmoji = true;

    auto pair = emojiGlyphInfoMap.emplace(glyphIndex, glyphInfo);
    return &pair.first->second;

  } 
  // non-emoji glyphs
  {
    size_t bytesPerRow = (bitmapWidth + 3) & ~3;
    std::vector<uint8_t> buf(bytesPerRow * bitmapHeight, 0);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate(
      buf.data(), bitmapWidth, bitmapHeight, 8, bytesPerRow, cs, kCGImageAlphaNone
    );
    CGColorSpaceRelease(cs);

    CGContextSetShouldAntialias(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetAllowsFontSmoothing(context, false);

    CGContextSetGrayFillColor(context, 1.0, 1.0);
    CTFontDrawGlyphs(ctFont.get(), &glyph, &drawOrigin, 1, context);
    CGContextRelease(context);

    std::extents shape{bitmapHeight, bitmapWidth};
    std::array strides{bytesPerRow, 1uz};
    auto view = std::mdspan(buf.data(), std::layout_stride::mapping{shape, strides});

    glyphInfo.atlasRegion = textureAtlas.AddGlyph(view);

    auto pair = glyphInfoMap.emplace(glyphIndex, glyphInfo);
    return &pair.first->second;
  }
}
