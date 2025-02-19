#include "./font.hpp"
#include "./font_locator.hpp"
#include "utils/logger.hpp"
#include "utils/region.hpp"
#include "glm/gtx/string_cast.hpp"
#include <mdspan>

#include "freetype/ftmodapi.h"

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
Font::FromName(const FontDescriptorWithName& desc, float linespace, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  if (fontPath.empty()) {
    return std::unexpected("Failed to find font for: " + desc.name);
  }
  try {
    return Font(fontPath, desc.height, desc.width, linespace, dpiScale);
  } catch (const std::runtime_error& e) {
    return std::unexpected(e.what());
  }
}

Font::Font(
  std::string _path, float _height, float _width, float _linespace, float _dpiScale
)
    : path(std::move(_path)), height(_height), width(_width), linespace(_linespace),
      dpiScale(_dpiScale) {
  if (face = CreateFace(library, path.c_str(), 0); face == nullptr) {
    throw std::runtime_error("Failed to create FT_Face for: " + path);
  }

  // winding order is clockwise starting from top left
  int trueHeight = height * dpiScale;
  height = trueHeight / dpiScale; // round down to nearest trueSize

  int trueWidth = width * dpiScale;
  width = trueWidth / dpiScale; // round down to nearest trueWidth

  auto roundPixel = [this](float val) -> float {
    return int(val * dpiScale) / dpiScale;
  };
  linespace = roundPixel(linespace);
  float topLinespace = roundPixel(linespace / 2);

  FT_Set_Pixel_Sizes(face.get(), trueWidth, trueHeight);
  charSize.x = (face->size->metrics.max_advance >> 6) / dpiScale;
  charSize.y = (face->size->metrics.height >> 6) / dpiScale;
  ascender = (face->size->metrics.ascender >> 6) / dpiScale;

  charSize.y += linespace;
  ascender += topLinespace;

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

const GlyphInfo*
Font::GetGlyphInfo(char32_t charcode, TextureAtlas& textureAtlas) {
  auto glyphIndex = FT_Get_Char_Index(face.get(), charcode);

  if (glyphIndex == 0) {
    return nullptr;
  }

  auto it = glyphInfoMap.find(glyphIndex);
  if (it != glyphInfoMap.end()) {
    return &(it->second);
  }

  FT_Int32 loadFlags = FT_LOAD_DEFAULT;
  FT_Load_Glyph(face.get(), glyphIndex, loadFlags);
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

  FT_GlyphSlot slot = face->glyph;
  FT_Bitmap& bitmap = slot->bitmap;

  auto view = std::mdspan(bitmap.buffer, bitmap.rows, bitmap.width);
  auto region = textureAtlas.AddGlyph(view);

  auto pair = glyphInfoMap.emplace(
    glyphIndex,
    GlyphInfo{
      .localPoss = MakeRegion(
        {
          slot->bitmap_left / dpiScale,
          -slot->bitmap_top / dpiScale,
        },
        {
          bitmap.width / dpiScale,
          bitmap.rows / dpiScale,
        }
      ),
      .atlasRegion = region,
    }
  );

  return &(pair.first->second);
}
