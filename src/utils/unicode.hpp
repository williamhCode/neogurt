#pragma once 

#include <format>
#include <string>
#include <vector>

std::string Char32ToUtf8(char32_t unicode);
char32_t Utf8ToChar32(const std::string& utf8String);
std::u32string Utf8ToUtf32(const std::string& utf8String);

struct GraphemeInfo {
  std::string str;
  size_t u8Len;
  size_t u32Len;
};
// template <>
// struct std::formatter<GraphemeInfo> {
//   constexpr auto parse(auto& ctx) {
//     return ctx.begin();
//   }
//   auto format(const GraphemeInfo& col, auto& ctx) const {
//     return std::format_to(ctx.out(), "({}, {}, {})", col.str, col.u8Len, col.u32Len);
//   }
// };

std::vector<GraphemeInfo> SplitByGraphemes(const std::string& text);
