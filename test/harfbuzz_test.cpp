#include "utils/unicode.hpp"
#include <hb-cplusplus.hh>
#include <hb-ft.h>
#include <print>
#include <vector>
#include <string>

// ----------------------------------------------------------------
// helpers
// ----------------------------------------------------------------

static void shapeAndPrint(
  hb_font_t* hbFont,
  hb_buffer_t* hbBuffer,
  const std::string& text,
  const std::vector<hb_feature_t>& features
) {
  std::println("  input: \"{}\"", text);

  hb_buffer_reset(hbBuffer);
  hb_buffer_add_utf8(hbBuffer, text.c_str(), -1, 0, -1);
  hb_buffer_guess_segment_properties(hbBuffer);
  hb_shape(hbFont, hbBuffer, features.data(), (unsigned)features.size());

  unsigned len = hb_buffer_get_length(hbBuffer);
  hb_glyph_info_t*     infos = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
  hb_glyph_position_t* pos   = hb_buffer_get_glyph_positions(hbBuffer, nullptr);

  auto u32 = Utf8ToUtf32(text);

  for (unsigned i = 0; i < len; i++) {
    uint32_t glyphId     = infos[i].codepoint;
    uint32_t clusterStart = infos[i].cluster;

    // next cluster boundary (handles RTL too)
    uint32_t clusterEnd;
    if (i + 1 < len) {
      clusterEnd = infos[i + 1].cluster;
      if (clusterEnd < clusterStart) std::swap(clusterStart, clusterEnd); // RTL
    } else {
      clusterEnd = (uint32_t)u32.size();
    }
    int numCells = (int)(clusterEnd - clusterStart);

    char glyphName[64] = {};
    hb_font_get_glyph_name(hbFont, glyphId, glyphName, sizeof(glyphName));

    double xAdv = pos[i].x_advance / 64.0;

    if (numCells > 1) {
      std::println(
        "  [LIGATURE] glyph_id={:<6} name={:<30} cluster={}->{} num_cells={} x_advance={:g}",
        glyphId, glyphName, clusterStart, clusterEnd, numCells, xAdv
      );
    } else {
      std::println(
        "             glyph_id={:<6} name={:<30} cluster={}     num_cells={} x_advance={:g}",
        glyphId, glyphName, clusterStart, numCells, xAdv
      );
    }
  }
  std::println();
}

// ----------------------------------------------------------------
// emoji section (unchanged)
// ----------------------------------------------------------------

static void runEmojiTest() {
  std::println("=== Emoji Test ===");

  FT_Library library;
  FT_Init_FreeType(&library);

  FT_Face face;
  FT_New_Face(library, "/System/Library/Fonts/Apple Color Emoji.ttc", 0, &face);
  FT_Set_Char_Size(face, 20 * 64, 0, 72, 72);

  std::string text("🚶‍➡️");

  auto u32 = Utf8ToUtf32(text);
  for (auto c : u32) {
    std::print("{:#x} ", (uint32_t)c);
  }
  std::println();

  hb::unique_ptr<hb_font_t>   hbFont(hb_ft_font_create(face, nullptr));
  hb::unique_ptr<hb_buffer_t> hbBuffer(hb_buffer_create());

  hb_buffer_add_utf8(hbBuffer, text.c_str(), -1, 0, -1);
  hb_buffer_guess_segment_properties(hbBuffer);
  hb_shape(hbFont, hbBuffer, nullptr, 0);

  unsigned len  = hb_buffer_get_length(hbBuffer);
  hb_glyph_info_t*     info = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
  hb_glyph_position_t* pos  = hb_buffer_get_glyph_positions(hbBuffer, nullptr);

  std::println("Raw buffer contents:");
  for (unsigned i = 0; i < len; i++) {
    hb_codepoint_t gid     = info[i].codepoint;
    unsigned       cluster = info[i].cluster;
    double x_advance = pos[i].x_advance / 64.;
    double y_advance = pos[i].y_advance / 64.;
    double x_offset  = pos[i].x_offset  / 64.;
    double y_offset  = pos[i].y_offset  / 64.;

    char glyphname[64] = {};
    hb_font_get_glyph_name(hbFont, gid, glyphname, sizeof(glyphname));

    std::println(
      "glyph='{}'\tcluster={}\tadvance=({:g},{:g})\toffset=({:g},{:g})",
      glyphname, cluster, x_advance, y_advance, x_offset, y_offset
    );
  }

  uint32_t glyphIndex = info[0].codepoint;
  FT_Load_Glyph(face, glyphIndex, FT_LOAD_COLOR);
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

  FT_GlyphSlot slot   = face->glyph;
  FT_Bitmap&   bitmap = slot->bitmap;
  std::println("pixel_mode: {}", bitmap.pixel_mode);
  std::println("bitmap.width, bitmap.rows: {}, {}", bitmap.width, bitmap.rows);

  FT_Done_Face(face);
  FT_Done_FreeType(library);
  std::println();
}

// ----------------------------------------------------------------
// ligature section
// ----------------------------------------------------------------

static void runLigatureTest() {
  std::println("=== Ligature Test ===");

  // swap this path to whatever ligature font you have installed
  // FiraCode, Cascadia Code, Monaspace, etc. all work
  const char* fontPath = "/Users/williamhou/Library/Fonts/JetBrainsMono-Medium.otf";

  FT_Library library;
  FT_Init_FreeType(&library);

  FT_Face face;
  if (FT_New_Face(library, fontPath, 0, &face) != 0) {
    std::println("ERROR: could not load font: {}", fontPath);
    FT_Done_FreeType(library);
    return;
  }
  FT_Set_Pixel_Sizes(face, 0, 32);
  std::println("font: {}", fontPath);
  std::println();

  hb::unique_ptr<hb_font_t>   hbFont(hb_ft_font_create(face, nullptr));
  hb::unique_ptr<hb_buffer_t> hbBuffer(hb_buffer_create());

  // features that enable coding ligatures
  // calt = contextual alternates (most coding ligatures use this)
  // liga = standard ligatures (fi, fl, etc.)
  std::vector<hb_feature_t> features;
  hb_feature_t calt, liga;
  hb_feature_from_string("calt", -1, &calt);
  hb_feature_from_string("liga", -1, &liga);
  features.push_back(calt);
  features.push_back(liga);

  std::println("--- full lines of code ---");
  std::println();

  std::vector<std::string> lines = {
    "if (x == null || y != null && z >= 0) {",
    "    auto result = items.filter([](auto&& x) { return x != nullptr; });",
    "    for (int i = 0; i <= items.size() - 1; ++i) {",
    "        fn(i -> i * 2);",
    "    }",
    "} else if (flags & 0xFF == 0x00) {",
    "    auto ptr = std::make_unique<Foo>(args...);",
    "    // TODO: handle error => return early",
    "    int x = a !== b ? a << 2 : b >> 1;",
    "    std::cout << \"result: \" << x << std::endl;",
  };

  for (const auto& line : lines) {
    shapeAndPrint(hbFont.get(), hbBuffer.get(), line, features);
  }

  std::println("--- with calt+liga features (isolated sequences) ---");
  std::println();

  // common coding ligatures to test
  std::vector<std::string> sequences = {
    "->",   // arrow
    "=>",   // fat arrow
    "==",   // equality
    "===",  // strict equality
    "!=",   // not equal
    "!==",  // strict not equal
    "<=",   // less-or-equal
    ">=",   // greater-or-equal
    ">>",   // right shift
    "<<",   // left shift
    "||",   // or
    "&&",   // and
    "::",   // scope
    "..",   // range
    "...",  // ellipsis
    "//",   // comment
    "/*",   // block comment open
    "*/",   // block comment close
    // longer context — ligatures sometimes depend on surrounding chars
    "x -> y",
    "if (a == b)",
    "a != b",
  };

  for (const auto& seq : sequences) {
    shapeAndPrint(hbFont.get(), hbBuffer.get(), seq, features);
  }

  std::println("--- without any features (for comparison) ---");
  std::println();

  std::vector<std::string> compareSeqs = { "->", "==", "!=" };
  for (const auto& seq : compareSeqs) {
    shapeAndPrint(hbFont.get(), hbBuffer.get(), seq, {});
  }

  FT_Done_Face(face);
  FT_Done_FreeType(library);
}

// ----------------------------------------------------------------
// main
// ----------------------------------------------------------------

int main() {
  runEmojiTest();
  runLigatureTest();
}
