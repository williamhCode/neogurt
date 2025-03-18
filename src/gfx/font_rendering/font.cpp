#include "./font.hpp"
#include "./font_locator.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "glm/gtx/string_cast.hpp"
#include <cstdlib>
#include <mdspan>
#include <span>

#include "freetype/ftmodapi.h"
#include "freetype/tttables.h"

// #include <harfbuzz/hb.h>
// #include <harfbuzz/hb-ft.h>

using namespace wgpu;

static FT_FacePtr
CreateFace(FT_Library library, const char* filepath, FT_Long face_index) {
  FT_Face face;
  if (FT_New_Face(library, filepath, face_index, &face)) {
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

std::expected<Font, std::string>
Font::FromName(const FontDescriptorWithName& desc, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  if (fontPath.empty()) {
    return std::unexpected("Failed to find font for: " + desc.name);
  }
  try {
    return Font(fontPath, desc.height, desc.width, dpiScale);
  } catch (const std::runtime_error& e) {
    return std::unexpected(e.what());
  }
}

Font::Font(
  std::string _path, float _height, float _width, float _dpiScale
)
    : path(std::move(_path)), height(_height), width(_width), dpiScale(_dpiScale) {
  if (face = CreateFace(library, path.c_str(), 0); face == nullptr) {
    throw std::runtime_error("Failed to create FT_Face for: " + path);
  }

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
  char32_t charcode,
  TextureAtlas<false>& textureAtlas,
  TextureAtlas<true>& colorTextureAtlas
) {
  auto glyphIndex = FT_Get_Char_Index(face.get(), charcode);

  if (glyphIndex == 0) {
    return nullptr;
  }

  auto it = glyphInfoMap.find(glyphIndex);
  if (it != glyphInfoMap.end()) {
    return &(it->second);
  }

  FT_Load_Glyph(face.get(), glyphIndex, FT_LOAD_COLOR);
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

  FT_GlyphSlot slot = face->glyph;
  FT_Bitmap& bitmap = slot->bitmap;

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

  // NOTE: assuming pitch is positive here, glyph will be flipped vertically if negative
  // https://freetype.org/freetype2/docs/glyphs/glyphs-7.html
  if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
    std::extents shape{bitmap.rows, bitmap.width};
    std::array strides{std::abs(bitmap.pitch) / sizeof(uint32_t), 1uz};
    auto view = std::mdspan(
      // should be safe since freetype treats bitmap.buffer as typeless
      // so should be 32-bit aligned and in correct order for BGRA
      reinterpret_cast<uint32_t*>(bitmap.buffer),
      std::layout_stride::mapping{shape, strides}
    );
    glyphInfo.atlasRegion = colorTextureAtlas.AddGlyph(view);
    glyphInfo.isEmoji = true;

  } else {
    std::extents shape{bitmap.rows, bitmap.width};
    std::array strides{std::abs(bitmap.pitch) / sizeof(uint8_t), 1uz};
    auto view = std::mdspan(bitmap.buffer, std::layout_stride::mapping{shape, strides});

    glyphInfo.atlasRegion = textureAtlas.AddGlyph(view);
  }

  auto pair = glyphInfoMap.emplace(glyphIndex, glyphInfo);
  return &(pair.first->second);
}
