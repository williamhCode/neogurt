#include "./unicode.hpp"
#include "utf8/checked.h"
#include "utils/logger.hpp"
#include <cwchar>

std::string Char32ToUTF8(char32_t unicode) {
  std::string utf8String;
  try {
    utf8::append(unicode, utf8String);
  } catch (const std::exception& e) {
    LOG_ERR("UnicodeToUTF8: {}, {}", e.what(), (uint32_t)unicode); 
    utf8String = "";
  }
  return utf8String;
}

char32_t UTF8ToChar32(const std::string& utf8String) {
  auto it = utf8String.begin();
  try {
    return utf8::next(it, utf8String.end());
  } catch (const std::exception& e) {
    LOG_ERR("UTF8ToUnicode: {}, {}", e.what(), utf8String);
    return 0;
  }
}

std::vector<std::string> SplitUTF8(const std::string& input) {
  std::vector<std::string> outputChars;
  auto it = input.begin();

  while (it != input.end()) {
    auto start = it;
    char32_t codepoint = utf8::next(it, input.end());
    outputChars.emplace_back(start, it);
    // add empty string if the character is double width
    for (int i = 0; i < wcwidth(codepoint) - 1; i++) {
      outputChars.emplace_back("");
    }
  }

  return outputChars;
}
