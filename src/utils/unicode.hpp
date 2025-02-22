#pragma once 

#include <string>
#include <vector>

std::string Char32ToUtf8(char32_t unicode);
char32_t Utf8ToChar32(const std::string& utf8String);

struct GraphemeInfo {
  std::string str;
  size_t u8Len;
  size_t u32Len;
};
std::vector<GraphemeInfo> SplitByGraphemes(const std::string& text);

