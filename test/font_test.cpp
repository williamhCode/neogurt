#define BOOST_TEST_MODULE FontTest
#include <boost/test/included/unit_test.hpp>

#include "gfx/font_rendering/font_locator.hpp"

BOOST_AUTO_TEST_CASE(FontPathResolvesCorrectly) {
  FontDescriptorWithName desc{.name = "Andale Mono"};
  std::string path = GetFontPathFromName(desc);

  BOOST_TEST_MESSAGE("Resolved font path: " << path);
  BOOST_CHECK(!path.empty());
  BOOST_CHECK(path.ends_with(".ttf") || path.ends_with(".otf"));
}
