#define BOOST_TEST_MODULE FontTest
#include <boost/test/included/unit_test.hpp>

#include "gfx/font_rendering/font_locator.hpp"
#include "editor/font.hpp"

#include "SDL3/SDL_init.h"
#include "app/path.hpp"
#include "app/sdl_window.hpp"
#include "session/options.hpp"

BOOST_AUTO_TEST_CASE(NormalFont) {
  FontDescriptorWithName desc{.name = "Andale Mono"};
  std::string path = GetFontPathFromName(desc);

  BOOST_TEST_MESSAGE("Resolved font path: " << path);
  BOOST_CHECK(!path.empty());
}

BOOST_AUTO_TEST_CASE(EmojiFont) {
  FontDescriptorWithName desc{.name = "Apple Color Emoji"};
  std::string path = GetFontPathFromName(desc);

  BOOST_TEST_MESSAGE("Resolved font path: " << path);
  BOOST_CHECK(!path.empty());
}

WGPUContext ctx;

BOOST_AUTO_TEST_CASE(Guifont) {
  SetupPaths();
  SDL_Init(SDL_INIT_VIDEO);
  FtInit();

  GlobalOptions globalOpts;
  sdl::Window window({1200, 800}, "Neogurt", globalOpts);

  auto fontFamilyResult = FontFamily::FromGuifont("Andale Mono,Apple Color Emoji:h15", 0, 1);
  if (!fontFamilyResult) {
    BOOST_TEST_MESSAGE(fontFamilyResult.error().what());
  }
  BOOST_CHECK(fontFamilyResult.has_value());
}
