#include "./highlight.hpp"
#include "utils/logger.hpp"

glm::vec4 GetDefaultBackground(const HlTable& table) {
  auto it = table.find(0);
  if (it == table.end()) {
    return {0, 0, 0, 1};
  }
  return it->second.background.or_else([&] {
    LOG_ERR("GetDefaultBackground: default highlight table entry (0) has no background color");
    return std::make_optional<glm::vec4>(0, 0, 0, 1);
  }).value();
}

glm::vec4 GetForeground(const HlTable& table, const Highlight& hl) {
  return hl.foreground.or_else([&] {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetForeground: default highlight table entry (0) not found");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    }
    return it->second.foreground.or_else([&] {
      LOG_ERR("GetForeground: default highlight table entry (0) has no foreground color");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    });
  }).value();
}

glm::vec4 GetBackground(const HlTable& table, const Highlight& hl) {
  return hl.background.or_else([&] {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetBoreground: default highlight table entry (0) not found");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    }
    return it->second.background.or_else([&] {
      LOG_ERR("GetBoreground: default highlight table entry (0) has no background color");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    });
  }).value();
}

glm::vec4 GetSpecial(const HlTable& table, const Highlight& hl) {
  return hl.special.or_else([&] {
    auto it = table.find(0);
    if (it == table.end()) {
      LOG_ERR("GetSpecial: default highlight table entry (0) not found");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    }
    // use foreground cuz special color is weird
    return it->second.foreground.or_else([&] {
      LOG_ERR("GetSpecial: default highlight table entry (0) has no foreground color");
      return std::make_optional<glm::vec4>(0, 0, 0, 1);
    });
  }).value();
}
