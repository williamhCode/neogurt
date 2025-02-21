#include "./unicode.hpp"
#include "utf8/checked.h"
#include "utils/logger.hpp"
#include <string>
#include <cwchar>

// converts a char32_t value to a utf8 string
std::string Char32ToUtf8(char32_t unicode) {
  std::string utf8String;
  try {
    utf8::append(unicode, utf8String);
  } catch (const std::exception& e) {
    LOG_ERR("Char32ToUtf8: {}, {}", e.what(), (uint32_t)unicode); 
    utf8String = "";
  }
  return utf8String;
}

// converts the first unicode character in a utf8 string to a char32_t value
char32_t Utf8ToChar32(std::string_view utf8String) {
  const auto *it = utf8String.begin();
  try {
    return utf8::next(it, utf8String.end());
  } catch (const std::exception& e) {
    LOG_ERR("Utf8ToChar32: {}, {}", e.what(), utf8String);
    return 0;
  }
}

// TODO: merge ligatures and multi-unicode emojis
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
