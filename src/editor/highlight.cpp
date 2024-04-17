#include "highlight.hpp"
#include "utils/logger.hpp"

glm::vec4 GetDefaultBackground(const HlTable& table) {
  auto it = table.find(0);
  if (it == table.end()) {
    return {0, 0, 0, 1};
  }
  return it->second.background.value();
}

glm::vec4 GetForeground(const HlTable& table, const Highlight& hl) {
  return hl.foreground.value_or([&]() {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetForeground: default highlight table entry (0) not found");
      return glm::vec4(0, 0, 0, 1);
    }
    return it->second.foreground.value();
  }());
}

glm::vec4 GetBackground(const HlTable& table, const Highlight& hl) {
  return hl.background.value_or([&]() {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetBackground: default highlight table entry (0) not found");
      return glm::vec4(0, 0, 0, 1);
    }
    return it->second.background.value();
  }());
}

glm::vec4 GetSpecial(const HlTable& table, const Highlight& hl) {
  return hl.special.value_or([&]() {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetSpecial: default highlight table entry (0) not found");
      return glm::vec4(0, 0, 0, 1);
    }
    return it->second.special.value();
  }());
}
