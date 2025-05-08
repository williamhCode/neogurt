#pragma once
#include <optional>
#include <string>

// Ui related options sent by "option_set" event.
// Events will be sent at startup and when options are changed.
// Some events can be ignored (not very significant).
// Using optional for lazy evaluation.
struct UiOptions {
  std::optional<std::string> guifont;
  std::optional<int> linespace = 0;
  std::optional<bool> mousehide = true;
};
