#include "gfx/font_rendering/font_locator.hpp"
#include <blend2d.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
  BLImage img(480, 480, BL_FORMAT_PRGB32);
  BLContext ctx(img);
  ctx.clearAll();

  std::string fontPath = GetFontPathFromName({.name = "JetBrains Mono"});
  const char regularText[] = "Hello World!";

  // Load font-face and handle a possible error.
  BLFontFace face;
  BLResult result = face.createFromFile(fontPath.c_str());
  if (result != BL_SUCCESS) {
    printf("Failed to load a font (err=%u)\n", result);
    return 1;
  }

  BLFont font;
  font.createFromFace(face, 50.0f);

  ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
  ctx.fillUtf8Text(BLPoint(60, 80), font, regularText);

  ctx.end();

  img.writeToFile("blend2d_test.png");
  return 0;
}
