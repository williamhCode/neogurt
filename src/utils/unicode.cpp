#include "unicode.hpp"
#include "utf8/checked.h"

std::string UnicodeToUTF8(uint32_t unicode) {
  std::string utf8String;
  utf8::append(unicode, utf8String);
  return utf8String;
}

uint32_t UTF8ToUnicode(const std::string& utf8String) {
  auto it = utf8String.begin();
  return utf8::next(it, utf8String.end());
}
