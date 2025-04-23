#include "utils/unicode.hpp"
#include <hb-cplusplus.hh>
#include <hb-ft.h>
#include <print>

int main() {
  FT_Library library;
  FT_Init_FreeType(&library);

  FT_Face face;
  FT_New_Face(library, "/System/Library/Fonts/Apple Color Emoji.ttc", 0, &face);
  // FT_New_Face(library, "/Users/williamhou/Library/Fonts/JetBrainsMono-Medium.otf", 0, &face);
  FT_Set_Char_Size(face, 20*64, 0, 72, 72);

  // std::string text("Hello, world!");
  std::string text("🏳️‍⚧️");

  auto u32 = Utf8ToUtf32(text);
  for (auto c : u32) {
    std::print("{:#x} ", (uint32_t)c);
  }
  std::println();

  hb::unique_ptr<hb_font_t> font(hb_ft_font_create(face, nullptr));

  hb::unique_ptr<hb_buffer_t> buffer(hb_buffer_create());
  hb_buffer_add_utf8(buffer, text.c_str(), -1, 0, -1);
  hb_buffer_guess_segment_properties(buffer);

  hb_shape(font, buffer, nullptr, 0);

  /* Get glyph information and positions out of the buffer. */
  unsigned int len = hb_buffer_get_length(buffer);
  hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, nullptr);
  hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, nullptr);

  /* Print them out as is. */
  std::println("Raw buffer contents:");
  for (unsigned int i = 0; i < len; i++) {
    hb_codepoint_t gid = info[i].codepoint;
    unsigned int cluster = info[i].cluster;
    double x_advance = pos[i].x_advance / 64.;
    double y_advance = pos[i].y_advance / 64.;
    double x_offset = pos[i].x_offset / 64.;
    double y_offset = pos[i].y_offset / 64.;

    char glyphname[64] = {};
    hb_font_get_glyph_name(font, gid, glyphname, sizeof(glyphname));

    std::println(
      "glyph='{}'\tcluster={}\tadvance=({:g},{:g})\toffset=({:g},{:g})", glyphname,
      cluster, x_advance, y_advance, x_offset, y_offset
    );
    std::println("gid: {} cluster: {}", info[i].codepoint, info[i].cluster);
  }

  std::println ("Converted to absolute positions:");
  /* And converted to absolute positions. */
  {
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++) {
      hb_codepoint_t gid = info[i].codepoint;
      unsigned int cluster = info[i].cluster;
      double x_position = current_x + pos[i].x_offset / 64.;
      double y_position = current_y + pos[i].y_offset / 64.;

      char glyphname[64] = {};
      hb_font_get_glyph_name(font, gid, glyphname, sizeof(glyphname));

      std::println(
        "glyph='{}' cluster={}  position=({:g},{:g})", glyphname, cluster, x_position,
        y_position
      );

      current_x += pos[i].x_advance / 64.;
      current_y += pos[i].y_advance / 64.;
    }
  }
}
