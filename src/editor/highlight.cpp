#include "./highlight.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"
#include "glm/gtx/string_cast.hpp"

static int VariantAsInt(const msgpack::type::variant& v) {
  if (v.is_uint64_t()) return v.as_uint64_t();
  if (v.is_int64_t()) return v.as_int64_t();
  LOG_ERR("VariantAsInt: variant is not convertible to int");
  return 0;
}

Highlight Highlight::FromDesc(const event::HlAttrMap& hlDesc) {
  Highlight hl{};

  for (const auto& [key, value] : hlDesc) {
    if (key == "foreground" || key == "fg") {
      hl.foreground = IntToColor(VariantAsInt(value));
    } else if (key == "background" || key == "bg") {
      hl.background = IntToColor(VariantAsInt(value));
    } else if (key == "special" || key == "sp") {
      hl.special = IntToColor(VariantAsInt(value));
    } else if (key == "reverse") {
      hl.reverse = value.as_bool();
    } else if (key == "italic") {
      hl.italic = value.as_bool();
    } else if (key == "bold") {
      hl.bold = value.as_bool();
    } else if (key == "strikethrough") {
      hl.strikethrough = value.as_bool();
    } else if (key == "underline") {
      hl.underline = UnderlineType::Underline;
    } else if (key == "undercurl") {
      hl.underline = UnderlineType::Undercurl;
    } else if (key == "underdouble") {
      hl.underline = UnderlineType::Underdouble;
    } else if (key == "underdotted") {
      hl.underline = UnderlineType::Underdotted;
    } else if (key == "underdashed") {
      hl.underline = UnderlineType::Underdashed;
    } else if (key == "blend") {
      hl.bgAlpha = 1 - (VariantAsInt(value) / 100.0f);
    } else if (key == "url") {
      // TODO: make urls clickable
      hl.url = value.as_string();
    } else if (key == "nocombine") {
      // NOTE: ignore for now
    }
  }
  if (hl.background) {
    hl.background->a = hl.bgAlpha;
  }

  return hl;
}

HlManager::HlManager() {
  auto& hl = hlTable[0];
  hl.foreground = {0, 0, 0, 1};
  hl.background = {0, 0, 0, 1};
  hl.special = {0, 0, 0, 1};
  defaultBg = {0, 0, 0, 1};
}

void HlManager::DefaultColorsSet(const event::DefaultColorsSet& e) {
  auto& hl = hlTable[0];
  hl.foreground = IntToColor(e.rgbFg);
  if (hl.bgAlpha >= 1) {
    hl.background = IntToColor(e.rgbBg);
  }
  defaultBg = IntToColor(e.rgbBg);
  hl.special = IntToColor(e.rgbSp);
}

void HlManager::HlAttrDefine(const event::HlAttrDefine& e) {
  hlTable[e.id] = Highlight::FromDesc(e.rgbAttrs);
}

void HlManager::SetOpacity(float opacity, int bgColor) {
  auto& hl = hlTable[0];
  if (opacity >= 1) {
    hl.background = defaultBg;
    hl.bgAlpha = 1;
  } else {
    hl.background = IntToColor(bgColor);
    hl.bgAlpha = std::clamp(opacity, 0.0f, 1.0f);
    hl.background->a = hl.bgAlpha;
  }
}

glm::vec4 HlManager::GetDefaultBackground() {
  return hlTable.at(0).background.value();
}

glm::vec4 HlManager::GetForeground(const Highlight& hl) {
  if (hl.reverse) {
    return hl.background.value_or(hlTable.at(0).background.value());
  }
  return hl.foreground.value_or(hlTable.at(0).foreground.value());
}

glm::vec4 HlManager::GetBackground(const Highlight& hl) {
  if (hl.reverse) {
    return hl.foreground.value_or(hlTable.at(0).foreground.value());
  }
  return hl.background.value_or(hlTable.at(0).background.value());
}

glm::vec4 HlManager::GetSpecial(const Highlight& hl) {
  // use foreground if no special
  return hl.special.value_or(GetForeground(hl));
}
