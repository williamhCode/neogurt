#include "input.hpp"

#include "SDL_events.h"
#include "app/options.hpp"
#include "editor/window.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <set>

InputHandler::InputHandler(Nvim& nvim, WinManager& winManager, bool macOptAsAlt, bool multigrid)
    : nvim(nvim), winManager(winManager), macOptAsAlt(macOptAsAlt), multigrid(multigrid) {
}

const std::set<SDL_Keycode> specialKeys{
  SDLK_SPACE,  SDLK_UP,     SDLK_DOWN,     SDLK_LEFT,     SDLK_RIGHT, SDLK_BACKSPACE,
  SDLK_DELETE, SDLK_END,    SDLK_RETURN,   SDLK_ESCAPE,   SDLK_TAB,   SDLK_F1,
  SDLK_F2,     SDLK_F3,     SDLK_F4,       SDLK_F5,       SDLK_F6,    SDLK_F7,
  SDLK_F8,     SDLK_F9,     SDLK_F10,      SDLK_F11,      SDLK_F12,   SDLK_HOME,
  SDLK_INSERT, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_KP_ENTER,
};

void InputHandler::HandleKeyboard(const SDL_KeyboardEvent& event) {
  auto key = event.keysym.sym;
  mod = (SDL_Keymod)event.keysym.mod;

  if (event.state == SDL_PRESSED) {
    bool isText = !specialKeys.contains(key);
    bool onlyShift = (mod ^ SDL_KMOD_LSHIFT) == 0 || (mod ^ SDL_KMOD_RSHIFT) == 0;
    bool onlyMod = key >= SDLK_LCTRL && key <= SDLK_RGUI;

    // dont process keys if
    // 1. is text with no mod keys
    // 2. is text with only shift mod
    // 3. only mod keys are pressed
    if ((isText && (!mod || onlyShift)) || onlyMod) return;

    if (!macOptAsAlt && (mod & SDL_KMOD_ALT) && isText) return;

    // reset text input to prevent ime triggered from option key
    if (macOptAsAlt && (mod & SDL_KMOD_ALT)) {
      SDL_StopTextInput();
      SDL_StartTextInput();
    }

    std::string keyName = SDL_GetKeyName(key);
    if (keyName.empty()) return;

    std::ranges::transform(keyName, keyName.begin(), [](unsigned char c) {
      return std::tolower(c);
    });

    std::string inputStr = "<";
    if (mod & SDL_KMOD_CTRL) inputStr += "C-";
    if (mod & SDL_KMOD_ALT) inputStr += "M-";
    if (mod & SDL_KMOD_GUI) inputStr += "D-";
    if (mod & SDL_KMOD_SHIFT) inputStr += "S-";
    inputStr += keyName + ">";

    // LOG_INFO("Key: {}", inputStr);
    nvim.Input(inputStr);
  }
}

void InputHandler::HandleTextInput(const SDL_TextInputEvent& event) {
  if (macOptAsAlt && (mod & SDL_KMOD_ALT)) return;

  std::string inputStr = event.text;
  if (inputStr == " ") return;
  if (inputStr == "<") inputStr = "<lt>";

  // LOG_INFO("Text: {}", inputStr);
  nvim.Input(inputStr);
}

void InputHandler::HandleMouseButton(const SDL_MouseButtonEvent& event) {
  if (event.state == SDL_PRESSED) {
    mouseButton = event.button;
  } else {
    mouseButton.reset();
    currGrid.reset();
  }
  HandleMouseButtonAndMotion(event.state, {event.x, event.y});
}

void InputHandler::HandleMouseMotion(const SDL_MouseMotionEvent& event) {
  HandleMouseButtonAndMotion(-1, {event.x, event.y});
}

void InputHandler::HandleMouseButtonAndMotion(int state, glm::vec2 mousePos) {
  if (!mouseButton.has_value()) return;

  int button = *mouseButton;
  std::string buttonStr;
  switch (button) {
    case SDL_BUTTON_LEFT: buttonStr = "left"; break;
    case SDL_BUTTON_MIDDLE: buttonStr = "middle"; break;
    case SDL_BUTTON_RIGHT: buttonStr = "right"; break;
    default: return;
  }

  std::string actionStr;
  switch (state) {
    case SDL_PRESSED: actionStr = "press"; break;
    case SDL_RELEASED: actionStr = "release"; break;
    case -1: actionStr = "drag"; break;
    default: return;
  }

  std::string modStr;
  if (mod & SDL_KMOD_CTRL) modStr += "C-";
  if (mod & SDL_KMOD_ALT) modStr += "M-";
  if (mod & SDL_KMOD_GUI) modStr += "D-";
  if (mod & SDL_KMOD_SHIFT) modStr += "S-";

  MouseInfo info;
  if (!currGrid.has_value()) {
    info = winManager.GetMouseInfo(mousePos);
    currGrid = info.grid;
  } else {
    info = winManager.GetMouseInfo(*currGrid, mousePos);
  }
  if (!multigrid) info.grid = 0;

  nvim.InputMouse(buttonStr, actionStr, modStr, info.grid, info.row, info.col);
}

void InputHandler::HandleMouseWheel(const SDL_MouseWheelEvent& event) {
  std::string modStr;
  if (mod & SDL_KMOD_CTRL) modStr += "C-";
  if (mod & SDL_KMOD_ALT) modStr += "M-";
  if (mod & SDL_KMOD_GUI) modStr += "D-";
  if (mod & SDL_KMOD_SHIFT) modStr += "S-";

  MouseInfo info;
  glm::vec2 mousePos(event.mouse_x, event.mouse_y);
  if (!currGrid.has_value()) {
    info = winManager.GetMouseInfo(mousePos);
  } else {
    info = winManager.GetMouseInfo(*currGrid, mousePos);
  }
  if (!multigrid) info.grid = 0;

  double scrollSpeed = 1;
  double scrollUnit = 1 / scrollSpeed;

  double yAbs = std::abs(event.y);
  double xAbs = std::abs(event.x);

  bool ypositive = event.y > 0;
  bool xpositive = event.x > 0;

  if (yAbs > xAbs) {
    xAccum = 0;
    if ((ypositive && scrollDir == -1) || (!ypositive && scrollDir == 1)) {
      yAccum = 0;
    }
    scrollDir = ypositive ? 1 : -1;

    yAccum += yAbs;
    yAccum = std::min(yAccum, 100.0);

    std::string actionStr = ypositive ? "up" : "down";
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

    std::string actionStr = xpositive ? "right" : "left";
    while (xAccum >= scrollUnit) {
      nvim.InputMouse("wheel", actionStr, modStr, info.grid, info.row, info.col);
      xAccum -= scrollUnit;
    }
  }
}
