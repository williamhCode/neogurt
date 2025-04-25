#include "utils/logger.hpp"
#include "utils/unicode.hpp"
#include <freetype/freetype.h>

int main() {
  FT_Library library;
  FT_Init_FreeType(&library);

  FT_Face face;
  FT_New_Face(library, "/System/Library/Fonts/Apple Color Emoji.ttc", 0, &face);
  // FT_New_Face(library, "/Users/williamhou/Library/Fonts/JetBrainsMono-Medium.otf", 0, &face);

  FT_Set_Char_Size(face, 20*64, 0, 72, 72);
  auto charcode = Utf8ToChar32("😀");
  FT_Error er = FT_Load_Char(face, charcode, FT_LOAD_COLOR);
  if (er != 0)
  {
    LOG_INFO("er: {}", er);
    const auto* es = FT_Error_String(er);
    LOG_INFO("error: {}", es);
  }
  FT_Bitmap& bitmap = face->glyph->bitmap;
  LOG_INFO("charcode: {:#x}", (uint32_t)charcode);
  LOG_INFO("pixel_mode: {}", bitmap.pixel_mode);
  LOG_INFO("bitmap.width, bitmap.rows: {}, {}", bitmap.width, bitmap.rows);

  return 0;
}

