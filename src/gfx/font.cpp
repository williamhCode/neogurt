#include "font.hpp"
#include "utils/logger.hpp"
#include "gfx/font/locator.hpp"
#include "utils/region.hpp"
#include "glm/gtx/string_cast.hpp"
#include <mdspan>

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
  return FT_Init_FreeType(&library);
}

void FtDone() {
  FT_Done_FreeType(library);
}

std::expected<Font, std::string>
Font::FromName(const FontDescriptorWithName& desc, float dpiScale) {
  auto fontPath = GetFontPathFromName(desc);
  // LOG("Font path: {}, size: {}", fontPath, desc.size);
  if (fontPath.empty()) {
    return std::unexpected("Failed to find font for: " + desc.name);
  }

  try {
    return Font(fontPath, desc.size, desc.width, dpiScale);
  } catch (const std::runtime_error& e) {
    return std::unexpected(e.what());
  }
}

Font::Font(std::string _path, int _size, int _width, float _dpiScale)
    : path(std::move(_path)), size(_size), width(_width), dpiScale(_dpiScale) {
  if (face = CreateFace(library, path.c_str(), 0); face == nullptr) {
    throw std::runtime_error("Failed to create FT_Face for: " + path);
  }

  // winding order is clockwise starting from top left
  trueSize = size * dpiScale;
  int trueWidth = width * dpiScale;
  FT_Set_Pixel_Sizes(face.get(), trueWidth, trueSize);

  charSize.x = (face->size->metrics.max_advance >> 6) / dpiScale;
  charSize.y = (face->size->metrics.height >> 6) / dpiScale;
  // LOG_INFO("Font: {}, size: {}, dpiScale: {}, charSize: {}",
  //          path, size, dpiScale, glm::to_string(charSize));
}

const Font::GlyphInfo*
Font::GetGlyphInfo(FT_ULong charcode, TextureAtlas& textureAtlas) {
  auto glyphIndex = FT_Get_Char_Index(face.get(), charcode);

  if (glyphIndex == 0) {
    return nullptr;
  }

  auto it = glyphInfoMap.find(glyphIndex);
  if (it != glyphInfoMap.end()) {
    return &(it->second);
  }

  // TODO: implement custom box drawing characters
  // if (charcode >= 0x2500 && charcode <= 0x25FF) {
  //   vertOffset = charSize.y * 0.2;
  //   vertOffset = floor(vertOffset * dpiScale) / dpiScale;
  // }

  FT_Load_Glyph(face.get(), glyphIndex, FT_LOAD_RENDER);
  auto& glyph = *(face->glyph);
  auto& bitmap = glyph.bitmap;

  auto region = textureAtlas.AddGlyph({
    bitmap.buffer,
    std::dextents<uint, 2>{bitmap.rows, bitmap.width},
  });

  auto pair = glyphInfoMap.emplace(
    glyphIndex,
    GlyphInfo{
      .sizePositions = MakeRegion(
        {0, 0},
        {
          bitmap.width / dpiScale,
          bitmap.rows / dpiScale,
        }
      ),
      .bearing =
        glm::vec2{
          glyph.bitmap_left / dpiScale,
          glyph.bitmap_top / dpiScale,
        },
      // .advance = (glyph.advance.x >> 6) / dpiScale,
      .region = region,
    }
  );

  return &(pair.first->second);
}
