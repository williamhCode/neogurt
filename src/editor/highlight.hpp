#pragma once

#include "glm/ext/vector_float4.hpp"
#include <optional>
#include <unordered_map>

enum class UnderlineType : uint8_t {
  Underline,
  Undercurl,
  Underdouble,
  Underdotted,
  Underdashed,
};

struct Highlight;

using HlTable = std::unordered_map<int, Highlight>;

struct Highlight {
  std::optional<glm::vec4> foreground;
  std::optional<glm::vec4> background;
  std::optional<glm::vec4> special;
  bool reverse;
  bool italic;
  bool bold;
  bool strikethrough;
  std::optional<UnderlineType> underline;
  float bgAlpha = 1; // 0 - 1
};

inline glm::vec4 GetForeground(const HlTable& table, const Highlight& hl) {
  return hl.foreground.value_or(table.at(0).foreground.value());
}

inline glm::vec4 GetBackground(const HlTable& table, const Highlight& hl) {
  return hl.background.value_or(table.at(0).background.value());
}

inline glm::vec4 GetSpecial(const HlTable& table, const Highlight& hl) {
  return hl.special.value_or(table.at(0).special.value());
}
