#include "input.hpp"

#include <cctype>
#include <optional>
#include <unordered_map>
#include "SDL_events.h"
#include "app/options.hpp"
#include "editor/window.hpp"
#include "utils/unicode.hpp"

InputHandler::InputHandler(Nvim& nvim, WinManager& winManager, glm::vec2 cursorPos)
    : nvim(nvim), winManager(winManager), cursorPos(cursorPos) {
}

// clang-format off
const std::unordered_map<int, std::string> specialKeys{
  {SDLK_SPACE, "Space"},
  {SDLK_UP, "Up"},
  {SDLK_DOWN, "Down"},
  {SDLK_LEFT, "Left"},
  {SDLK_RIGHT, "Right"},
  {SDLK_BACKSPACE, "BS"},
  {SDLK_DELETE, "Del"},
  {SDLK_END, "End"},
  {SDLK_RETURN, "CR"},
  {SDLK_ESCAPE, "ESC"},
  {SDLK_TAB, "Tab"},
  {SDLK_F1, "F1"},
  {SDLK_F2, "F2"},
  {SDLK_F3, "F3"},
  {SDLK_F4, "F4"},
  {SDLK_F5, "F5"},
  {SDLK_F6, "F6"},
  {SDLK_F7, "F7"},
  {SDLK_F8, "F8"},
  {SDLK_F9, "F9"},
  {SDLK_F10, "F10"},
  {SDLK_F11, "F11"},
  {SDLK_F12, "F12"},
  {SDLK_HOME, "Home"},
  {SDLK_INSERT, "Insert"},
  {SDLK_PAGEUP, "PageUp"},
  {SDLK_PAGEDOWN, "PageDown"},
  {SDLK_KP_ENTER, "kEnter"},
};
// clang-format on

void InputHandler::HandleKey(SDL_KeyboardEvent event) {
  auto key = event.keysym.sym;
  auto mod = event.keysym.mod;

  if (event.state == SDL_PRESSED) {
    bool isSpecialKey = specialKeys.contains(key);

    bool onlyMod = key & SDLK_SCANCODE_MASK;
    if (onlyMod) return;

    bool noMod = mod == 0;

    // bool onlyShift = (mod ^ GLFW_MOD_SHIFT) == 0;
    // if ((!mod && !isSpecialKey) || onlyMod || (onlyShift && !isSpecialKey)) return;

    // don't process keys when options pressed if not using opt as alt
    // if (!options.macOptAsAlt && (mod & GLFW_MOD_ALT) && !isSpecialKey) return;

    auto keyName = isSpecialKey ? specialKeys.at(key) : SDL_GetKeyName(key);
    // std::string keyName = SDL_GetKeyName(key);
    if (keyName.empty()) return;

    std::string inputStr = keyName;
    if (!noMod) {
      auto temp = inputStr;
      inputStr = "<";
      if (mod & SDL_KMOD_CTRL) inputStr += "C-";
      if (mod & SDL_KMOD_ALT) inputStr += "M-";
      if (mod & SDL_KMOD_GUI) inputStr += "D-";
      if (mod & SDL_KMOD_SHIFT) inputStr += "S-";
      inputStr += temp + ">";
    } else if (isSpecialKey) {
      inputStr = "<" + inputStr + ">";
    }
    if (inputStr.size() == 1) {
      inputStr = std::tolower(inputStr[0]);
    }

    LOG_INFO("inputStr: {}", inputStr);

    nvim.Input(inputStr);
  }
}

void InputHandler::HandleChar(unsigned int codepoint) {
  
}

void InputHandler::HandleMouseButton(int button, int action, int /* mods */) {
  
}

void InputHandler::HandleCursorPos(double xpos, double ypos) {
  
}

void InputHandler::HandleMouseButtonAndCursorPos(int action) {
  
}

void InputHandler::HandleScroll(double xoffset, double yoffset) {
  
}
