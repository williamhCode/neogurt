#include "./highlight.hpp"
#include "utils/color.hpp"
#include "utils/logger.hpp"

static int VariantAsInt(const msgpack::type::variant& v) {
  if (v.is_uint64_t()) return v.as_uint64_t();
  if (v.is_int64_t()) return v.as_int64_t();
  LOG_ERR("VariantAsInt: variant is not convertible to int");
  return 0;
}

Highlight Highlight::FromDesc(const std::map<std::string, msgpack::type::variant>& hlDesc) {
  Highlight hl{};

  for (const auto& [key, value] : hlDesc) {
    if (key == "fg") {
      hl.foreground = IntToColor(VariantAsInt(value));
    } else if (key == "bg") {
      hl.background = IntToColor(VariantAsInt(value));
    } else if (key == "sp") {
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

void HlTableInit(HlTable& table, const Options& options) {
  auto& hl = table[0];
  hl.foreground = {0, 0, 0, 1};
  hl.background = {0, 0, 0, 1};
  hl.special = {0, 0, 0, 1};

  if (options.opacity < 1) {
    hl.background = IntToColor(options.bgColor);
    hl.background->a = options.opacity;
    hl.bgAlpha = options.opacity;
  }
}

glm::vec4 GetDefaultBackground(const HlTable& table) {
  return table.at(0).background.value();
}

glm::vec4 GetForeground(const HlTable& table, const Highlight& hl) {
  if (hl.reverse) {
    return hl.background.value_or(table.at(0).background.value());
  }
  return hl.foreground.value_or(table.at(0).foreground.value());
}

glm::vec4 GetBackground(const HlTable& table, const Highlight& hl) {
  if (hl.reverse) {
    return hl.foreground.value_or(table.at(0).foreground.value());
  }
  return hl.background.value_or(table.at(0).background.value());
}

glm::vec4 GetSpecial(const HlTable& table, const Highlight& hl) {
  return hl.special.value_or(table.at(0).special.value());
}
