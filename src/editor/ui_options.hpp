#pragma once
#include <optional>
#include <string>

// ui related options sent by "option_set" event.
// events will be sent at startup and when options are changed.
// some events can be ignored (not very significant).
struct UiOptions {
  std::optional<std::string> guifont;
  std::optional<int> linespace = 0;
  std::optional<bool> mousehide = true;
};
