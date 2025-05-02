#pragma once

#include "event/ui_parse.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/gtx/string_cast.hpp"
#include <optional>
#include <unordered_map>
#include <string>

enum class UnderlineType : uint32_t {
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
  std::string url;

  static Highlight FromDesc(const event::HlAttrMap& hlDesc);
};
template <>
struct std::formatter<Highlight> {
  constexpr auto parse(auto& ctx) {
    return ctx.begin();
  }
  auto format(const Highlight& col, auto& ctx) const {
    return std::format_to(
      ctx.out(), "(fg: {}, bg: {}, sp: {})", glm::to_string(col.foreground.value_or(glm::vec4(0))),
      glm::to_string(col.background.value_or(glm::vec4(0))),
      glm::to_string(col.special.value_or(glm::vec4(0)))
    );
  }
};

struct HlManager {
  std::unordered_map<int, Highlight> hlTable;
  glm::vec4 defaultBg;

  HlManager();
  void DefaultColorsSet(const event::DefaultColorsSet& e);
  void HlAttrDefine(const event::HlAttrDefine& e);

  void SetOpacity(float opacity, int bgColor);

  glm::vec4 GetDefaultBackground();
  glm::vec4 GetForeground(const Highlight& hl);
  glm::vec4 GetBackground(const Highlight& hl);
  glm::vec4 GetSpecial(const Highlight& hl);
};
