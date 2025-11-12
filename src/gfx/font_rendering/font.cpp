#include "./font.hpp"
#include "./font_locator.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "glm/gtx/string_cast.hpp"
#include <cstdlib>
#include <mdspan>
#include <span>
#include <unordered_set>

#include "freetype/ftmodapi.h"
#include "utils/unicode.hpp"

using namespace wgpu;

// Check if a character is emoji-by-default (no FE0F needed)
// These are fully-qualified emoji from emoji-test.txt that don't require U+FE0F
// Data source: docs/emoji/emoji-test.txt
static bool IsEmojiByDefault(char32_t c) {
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

  return emojiByDefault.contains(c);
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
    char32_t c = u32text[0];
    bool isTextDefaultRange =
      (c >= 0x2000 && c <= 0x27BF) ||  // Misc symbols, dingbats, arrows
      (c >= 0x2900 && c <= 0x2BFF) ||  // Misc math symbols, arrows, geometric
      (c >= 0x3000 && c <= 0x303F) ||  // CJK symbols
      (c >= 0x3200 && c <= 0x32FF);    // Enclosed CJK

    // Exclude emoji-by-default characters from text-default treatment
    if (isTextDefaultRange && !IsEmojiByDefault(c)) {
      hasTextSelector = true;
    }
  }

  // Reject glyph if variation selector doesn't match glyph type
  if (hasEmojiSelector && !isColorEmoji) {
    // Text requested emoji presentation but font returned text glyph
    return false;
  }
  if (hasTextSelector && isColorEmoji) {
    // Text requested text presentation but font returned emoji glyph
    return false;
  }

  return true;
}

static FT_FacePtr
CreateFace(FT_Library library, const char* filepath, FT_Long face_index) {
  FT_Face face;
  if (FT_New_Face(library, filepath, face_index, &face) != 0) {
    return nullptr;
  }
  return FT_FacePtr(face);
}

static FT_Library library;

int FtInit() {
  auto error = FT_Init_FreeType(&library);

  FT_Bool no_stem_darkening = false;
  FT_Property_Set(library, "autofitter", "no-stem-darkening", &no_stem_darkening);
  FT_Property_Set(library, "cff", "no-stem-darkening", &no_stem_darkening);
  FT_Property_Set(library, "type1", "no-stem-darkening", &no_stem_darkening);
  FT_Property_Set(library, "t1cid", "no-stem-darkening", &no_stem_darkening);

  FT_Int darken_params[8] = {500,  400,  // x1, y1
                             1000, 300,  // x2, y2
                             1667, 275,  // x3, y3
                             2333, 225}; // x4, y4
  FT_Property_Set(library, "autofitter", "darkening-parameters", darken_params);
  FT_Property_Set(library, "cff", "darkening-parameters", darken_params);
  FT_Property_Set(library, "type1", "darkening-parameters", darken_params);
  FT_Property_Set(library, "t1cid", "darkening-parameters", darken_params);

  return error;
}

void FtDone() {
  FT_Done_FreeType(library);
}

std::expected<Font, std::runtime_error>
Font::FromName(const FontDescriptorWithName& desc, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  if (fontPath.empty()) {
    return std::unexpected(std::runtime_error("Failed to find font for: " + desc.name));
  }
  try {
    return Font(fontPath, desc.height, desc.width, dpiScale);
  } catch (std::runtime_error& e) {
    return std::unexpected(std::move(e));
  }
}

Font::Font(
  std::string _path, float _height, float _width, float _dpiScale
)
    : path(std::move(_path)), height(_height), width(_width), dpiScale(_dpiScale) {
  if (face = CreateFace(library, path.c_str(), 0); face == nullptr) {
    throw std::runtime_error("Failed to create FT_Face for: " + path);
  }
  hbFont = hb::unique_ptr(hb_ft_font_create(face.get(), nullptr));
  hbBuffer = hb::unique_ptr(hb_buffer_create());

  // winding order is clockwise starting from top left
  int trueHeight = height * dpiScale;
  height = trueHeight / dpiScale; // round down to nearest trueSize

  int trueWidth = width * dpiScale;
  width = trueWidth / dpiScale; // round down to nearest trueWidth

  emojiRatio = 1;
  // check has color (color bitmap images)
  // if so, choose the next biggest size and set the proper ratio
  if (FT_HAS_COLOR(face)) {
    std::span bitmapSizes(face->available_sizes, face->num_fixed_sizes);
    for (size_t i = 0; i < bitmapSizes.size(); i++) {
      const FT_Bitmap_Size& size = bitmapSizes[i];

      int y_ppem = size.y_ppem >> 6;
      // choose the bitmap size if: equal size, at least 1.x size, or last size
      // at least 1.x size makes sure emoji is crisp
      if (y_ppem == trueHeight || y_ppem >= trueHeight * 1.2 ||
          i + 1 == bitmapSizes.size()) {
        emojiRatio = (float)trueHeight / y_ppem;

        trueHeight = y_ppem;
        trueWidth = 0;
        break;
      }
    }

    // for (const auto& size : bitmapSizes) {
    //   LOG_INFO(
    //     "Bitmap size: {}x{}, y_ppem: {}", size.width, size.height, size.y_ppem >> 6
    //   );
    // }
  }

  FT_Set_Pixel_Sizes(face.get(), trueWidth, trueHeight);
  charSize.x = (face->size->metrics.max_advance >> 6) / dpiScale;
  charSize.y = (face->size->metrics.height >> 6) / dpiScale;
  ascender = (face->size->metrics.ascender >> 6) / dpiScale;

  float y_scale = face->size->metrics.y_scale;
  underlinePosition = (FT_MulFix(face->underline_position, y_scale) >> 6) / dpiScale;
  underlineThickness = (FT_MulFix(face->underline_thickness, y_scale) >> 6) / dpiScale;
  underlineThickness = std::max(underlineThickness, 1.0f / dpiScale);

  // LOG_INFO(
  //   "Font: {}, size: {}, dpiScale: {}, charSize: {}, ascender: {}, underlinePosition: "
  //   "{}, underlineThickness: {}",
  //   path, height, dpiScale, glm::to_string(charSize), ascender, underlinePosition,
  //   underlineThickness
  // );
}

const GlyphInfo* Font::GetGlyphInfo(
  const std::string& text,
  TextureAtlas<false>& textureAtlas,
  TextureAtlas<true>& colorTextureAtlas
) {
  // look for cached glyphs
  auto it = glyphInfoMap.find(text);
  if (it != glyphInfoMap.end()) {
    return &(it->second);
  }

  it = emojiGlyphInfoMap.find(text);
  if (it != emojiGlyphInfoMap.end()) {
    return &(it->second);
  }

  // load glyph
  uint32_t glyphIndex = 0;
  std::u32string u32text = Utf8ToUtf32(text);

  if (u32text.size() == 1) {
    glyphIndex = FT_Get_Char_Index(face.get(), u32text[0]);

  } else { // shape the text if not single unicode character (e.g. ZWJ emojis)
    hb_buffer_reset(hbBuffer);
    hb_buffer_add_utf8(hbBuffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hbBuffer);
    hb_shape(hbFont, hbBuffer, nullptr, 0);

    uint len = hb_buffer_get_length(hbBuffer);
    if (len > 0) {
      hb_glyph_info_t* info = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
      glyphIndex = info[0].codepoint;
    }
  }

  if (glyphIndex == 0) return nullptr;

  FT_Load_Glyph(face.get(), glyphIndex, FT_LOAD_COLOR);
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

  FT_GlyphSlot slot = face->glyph;
  FT_Bitmap& bitmap = slot->bitmap;

  if (bitmap.width == 0 || bitmap.rows == 0) {
    return nullptr;
  }

  // Check emoji presentation rules
  bool isColorEmoji = (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);
  if (!ShouldAcceptGlyphForEmojiPresentation(u32text, isColorEmoji)) {
    return nullptr;
  }

  GlyphInfo glyphInfo{
    .localPoss = MakeRegion(
      {
        slot->bitmap_left / dpiScale * emojiRatio,
        -slot->bitmap_top / dpiScale * emojiRatio,
      },
      {
        bitmap.width / dpiScale * emojiRatio,
        bitmap.rows / dpiScale * emojiRatio,
      }
    ),
  };

  // emoji glyphs
  if (isColorEmoji) {
    std::extents shape{bitmap.rows, bitmap.width};
    // NOTE: assuming pitch is positive here, glyph will be flipped vertically if negative
    // https://freetype.org/freetype2/docs/glyphs/glyphs-7.html
    std::array strides{std::abs(bitmap.pitch) / sizeof(uint32_t), 1uz};
    auto view = std::mdspan(
      // should be safe since freetype treats bitmap.buffer as typeless
      // so should be 32-bit aligned and in correct order for BGRA
      reinterpret_cast<uint32_t*>(bitmap.buffer),
      std::layout_stride::mapping{shape, strides}
    );

    glyphInfo.atlasRegion = colorTextureAtlas.AddGlyph(view);
    glyphInfo.isEmoji = true;

    auto pair = emojiGlyphInfoMap.emplace(text, glyphInfo);
    return &(pair.first->second);
  }
  // non-emoji glyphs
  {
    std::extents shape{bitmap.rows, bitmap.width};
    std::array strides{std::abs(bitmap.pitch) / sizeof(uint8_t), 1uz};
    auto view = std::mdspan(bitmap.buffer, std::layout_stride::mapping{shape, strides});

    glyphInfo.atlasRegion = textureAtlas.AddGlyph(view);

    auto pair = glyphInfoMap.emplace(text, glyphInfo);
    return &(pair.first->second);
  }
}
