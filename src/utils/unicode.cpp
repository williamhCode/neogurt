#include "unicode.hpp"
#include "utf8/checked.h"
#include "utils/logger.hpp"

std::string UnicodeToUTF8(char32_t unicode) {
  std::string utf8String;
  try {
    utf8::append(unicode, utf8String);
  } catch (const std::exception& e) {
    LOG_ERR("UnicodeToUTF8: {}, {}", e.what(), (uint32_t)unicode); 
    utf8String = "";
  }
  return utf8String;
}

char32_t UTF8ToUnicode(const std::string& utf8String) {
  auto it = utf8String.begin();
  try {
    return utf8::next(it, utf8String.end());
  } catch (const std::exception& e) {
    LOG_ERR("UTF8ToUnicode: {}, {}", e.what(), utf8String);
    return 0;
  }
}
