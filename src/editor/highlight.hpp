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

using HlTable = std::unordered_map<int, Highlight>;

glm::vec4 GetDefaultBackground(const HlTable& table);
glm::vec4 GetForeground(const HlTable& table, const Highlight& hl);
glm::vec4 GetBackground(const HlTable& table, const Highlight& hl);
glm::vec4 GetSpecial(const HlTable& table, const Highlight& hl);
