#include "input.hpp"

#include "GLFW/glfw3.h"
#include <optional>
#include <unordered_map>
#include "app/options.hpp"
#include "editor/window.hpp"
#include "utils/unicode.hpp"

InputHandler::InputHandler(Nvim& nvim, WinManager& winManager, glm::vec2 cursorPos)
    : nvim(nvim), winManager(winManager), cursorPos(cursorPos) {
}

// clang-format off
const std::unordered_map<int, std::string> specialKeys{
  {GLFW_KEY_SPACE, "Space"},
  {GLFW_KEY_UP, "Up"},
  {GLFW_KEY_DOWN, "Down"},
  {GLFW_KEY_LEFT, "Left"},
  {GLFW_KEY_RIGHT, "Right"},
  {GLFW_KEY_BACKSPACE, "BS"},
  {GLFW_KEY_DELETE, "Del"},
  {GLFW_KEY_END, "End"},
  {GLFW_KEY_ENTER, "CR"},
  {GLFW_KEY_ESCAPE, "ESC"},
  {GLFW_KEY_TAB, "Tab"},
  {GLFW_KEY_F1, "F1"},
  {GLFW_KEY_F2, "F2"},
  {GLFW_KEY_F3, "F3"},
  {GLFW_KEY_F4, "F4"},
  {GLFW_KEY_F5, "F5"},
  {GLFW_KEY_F6, "F6"},
  {GLFW_KEY_F7, "F7"},
  {GLFW_KEY_F8, "F8"},
  {GLFW_KEY_F9, "F9"},
  {GLFW_KEY_F10, "F10"},
  {GLFW_KEY_F11, "F11"},
  {GLFW_KEY_F12, "F12"},
  {GLFW_KEY_HOME, "Home"},
  {GLFW_KEY_INSERT, "Insert"},
  {GLFW_KEY_PAGE_UP, "PageUp"},
  {GLFW_KEY_PAGE_DOWN, "PageDown"},
  {GLFW_KEY_KP_ENTER, "kEnter"},
};
// clang-format on

void InputHandler::HandleKey(int key, int scancode, int action, int _mods) {
  mods = _mods;

  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    bool isSpecialKey =
      (key >= GLFW_KEY_ESCAPE && key < GLFW_KEY_LEFT_SHIFT) || key == GLFW_KEY_SPACE;
    bool onlyMod = key >= GLFW_KEY_LEFT_SHIFT && key <= GLFW_KEY_RIGHT_SUPER;
    bool onlyShift = (mods ^ GLFW_MOD_SHIFT) == 0;
    if ((!mods && !isSpecialKey) || onlyMod || (onlyShift && !isSpecialKey)) return;
    // don't process keys when options pressed if not using opt as alt
    if (!options.macOptAsAlt && (mods & GLFW_MOD_ALT) && !isSpecialKey) return;

    auto keyName = isSpecialKey ? specialKeys.at(key) : glfwGetKeyName(key, 0);
    if (keyName == "") return;

    std::string inputStr = "<";
    if (mods & GLFW_MOD_CONTROL) inputStr += "C-";
    if (mods & GLFW_MOD_ALT) inputStr += "M-";
    if (mods & GLFW_MOD_SUPER) inputStr += "D-";
    if (mods & GLFW_MOD_SHIFT) inputStr += "S-";
    inputStr += keyName + ">";

    nvim.Input(inputStr);
  }
}

void InputHandler::HandleChar(unsigned int codepoint) {
  // don't process characters when options pressed if using opt as alt
  if (options.macOptAsAlt && (mods & GLFW_MOD_ALT)) return;

  // handle space in KeyInput
  if (codepoint == GLFW_KEY_SPACE) return;

  auto inputStr = UnicodeToUTF8(codepoint);
  nvim.Input(inputStr);
}

void InputHandler::HandleMouseButton(int button, int action, int mods) {
  if (action == GLFW_PRESS) {
    mouseButton = button;
  } else if (action == GLFW_RELEASE) {
    mouseButton = std::nullopt;
    currGrid = std::nullopt;
  }

  HandleMouseButtonAndCursorPos(action);
}

void InputHandler::HandleCursorPos(double xpos, double ypos) {
  cursorPos = {xpos, ypos};

  HandleMouseButtonAndCursorPos(-1);
}

void InputHandler::HandleMouseButtonAndCursorPos(int action) {
  if (!mouseButton.has_value()) return;

  int button = *mouseButton;
  std::string buttonStr;
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: buttonStr = "left"; break;
    case GLFW_MOUSE_BUTTON_RIGHT: buttonStr = "right"; break;
    case GLFW_MOUSE_BUTTON_MIDDLE: buttonStr = "middle"; break;
    default: return;
  }

  std::string actionStr;
  switch (action) {
    case GLFW_PRESS: actionStr = "press"; break;
    case GLFW_RELEASE: actionStr = "release"; break;
    case -1: actionStr = "drag"; break;
    default: assert(false);
  }

  std::string modStr = "";
  if (mods & GLFW_MOD_CONTROL) modStr += "C-";
  if (mods & GLFW_MOD_ALT) modStr += "M-";
  if (mods & GLFW_MOD_SUPER) modStr += "D-";
  if (mods & GLFW_MOD_SHIFT) modStr += "S-";

  MouseInfo info;
  if (!currGrid.has_value()) {
    info = winManager.GetMouseInfo(cursorPos);
    currGrid = info.grid;
  } else {
    info = winManager.GetMouseInfo(*currGrid, cursorPos);
  }

  nvim.InputMouse(buttonStr, actionStr, modStr, info.grid, info.row, info.col);
}

void InputHandler::HandleScroll(double xoffset, double yoffset) {
  std::string modStr = "";
  if (mods & GLFW_MOD_CONTROL) modStr += "C-";
  if (mods & GLFW_MOD_ALT) modStr += "M-";
  if (mods & GLFW_MOD_SUPER) modStr += "D-";
  if (mods & GLFW_MOD_SHIFT) modStr += "S-";

  MouseInfo info;
  if (!currGrid.has_value()) {
    info = winManager.GetMouseInfo(cursorPos);
  } else {
    info = winManager.GetMouseInfo(*currGrid, cursorPos);
  }

  double scrollSpeed = 1;
  double scrollUnit = 1 / scrollSpeed;

  double xAbs = std::abs(xoffset);
  double yAbs = std::abs(yoffset);

  bool ypositive = yoffset > 0;
  bool xpositive = xoffset > 0;

  if (yAbs > xAbs) {
    xAccum = 0;
    if ((ypositive && scrollDir == -1) || (!ypositive && scrollDir == 1)) {
      yAccum = 0;
    }
    scrollDir = ypositive ? 1 : -1;

    yAccum += yAbs;
    yAccum = std::min(yAccum, 100.0);

    auto actionStr = ypositive ? "up" : "down";
    while (yAccum >= scrollUnit) {
      nvim.InputMouse("wheel", actionStr, modStr, info.grid, info.row, info.col);
      yAccum -= scrollUnit;
    }
  } else if (xAbs > yAbs) {
    yAccum = 0;
    if ((xpositive && scrollDir == -1) || (!xpositive && scrollDir == 1)) {
      xAccum = 0;
    }
    scrollDir = xpositive ? 1 : -1;

    xAccum += xAbs;
    xAccum = std::min(xAccum, 100.0);

    auto actionStr = xpositive ? "left" : "right";
    while (xAccum >= scrollUnit) {
      nvim.InputMouse("wheel", actionStr, modStr, info.grid, info.row, info.col);
      xAccum -= scrollUnit;
    }
  }
}
