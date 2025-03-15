#include "./unicode.hpp"
#include "utils/logger.hpp"
#include <string>
#include <boost/locale/conversion.hpp>
#include <boost/locale/boundary.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/locale/utf.hpp>

// converts a char32_t value to a utf8 string
std::string Char32ToUtf8(char32_t unicode) {
  using namespace boost::locale::utf;
  std::string utf8String;
  utf_traits<char>::encode(unicode, std::back_inserter(utf8String));
  return utf8String;
}

// converts the first unicode character in a utf8 string to a char32_t value
char32_t Utf8ToChar32(const std::string& utf8String) {
  using namespace boost::locale::utf;
  auto it = utf8String.begin();
  auto codepoint = utf_traits<char>::decode(it, utf8String.end());
  if (codepoint == incomplete) {
    LOG_ERR("Utf8ToChar32 - incomplete string: {}", utf8String);
    codepoint = 0;
  }
  if (codepoint == illegal) {
    LOG_ERR("Utf8ToChar32 - illegal string: {}", utf8String);
    codepoint = 0;
  }
  return codepoint;
}

// copied from here
// https://github.com/tzlaine/text/blob/dd2959e7143fde3f62b24d87a6573b5b96b6ea46/include/boost/text/estimated_width.hpp
static int GetDisplayWidth(uint32_t cp) {
  using pair_type = std::pair<uint32_t, uint32_t>;
  constexpr pair_type double_wides[] = {
    {0x1100, 0x115f},   {0x2329, 0x232a},   {0x2e80, 0x303e},   {0x3040, 0xa4cf},
    {0xac00, 0xd7a3},   {0xf900, 0xfaff},   {0xfe10, 0xfe19},   {0xfe30, 0xfe6f},
    {0xff00, 0xff60},   {0xffe0, 0xffe6},   {0x1f300, 0x1f64f}, {0x1f900, 0x1f9ff},
    {0x20000, 0x2fffd}, {0x30000, 0x3fffd},
  };
  const auto* last = std::end(double_wides);
  const auto* it =
    std::lower_bound(std::begin(double_wides), last, cp, [](pair_type y, uint32_t x) {
      return y.second < x;
    });
  if (it != last && it->first <= cp && cp <= it->second) return 2;
  return 1;
}

// Get display width of a grapheme
static int GetGraphemeWidth(const std::string& grapheme) {
  return GetDisplayWidth(Utf8ToChar32(grapheme));
}

// splits a string (utf8) into a vector of strings,
// each containing a single grapheme
std::vector<GraphemeInfo> SplitByGraphemes(const std::string& text) {
  using namespace boost::locale::boundary;
  std::vector<GraphemeInfo> graphemes;

  static std::locale loc = boost::locale::generator().generate("en_US.UTF-8");
  ssegment_index index(character, text.begin(), text.end(), loc);

  for (const auto& i : index) {
    std::string utf8Str = i.str();
    std::u32string utf32Str = boost::locale::conv::utf_to_utf<char32_t>(utf8Str);

    if (GetGraphemeWidth(i.str()) >= 2) {
      graphemes.emplace_back(utf8Str, 0, 0);
      graphemes.emplace_back("", utf8Str.length(), utf32Str.length());
    } else {
      graphemes.emplace_back(utf8Str, utf8Str.length(), utf32Str.length());
    }
  }

  return graphemes;
}
