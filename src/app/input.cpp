#include "input.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "app/options.hpp"
#include "editor/window.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <set>

InputHandler::InputHandler(
  Nvim& nvim, WinManager& winManager, bool macOptAsAlt, bool multigrid
)
    : nvim(nvim), winManager(winManager), macOptIsMeta(macOptAsAlt),
      multigrid(multigrid) {
}

const std::set<SDL_Keycode> specialKeys{
  SDLK_SPACE,  SDLK_UP,     SDLK_DOWN,     SDLK_LEFT,     SDLK_RIGHT, SDLK_BACKSPACE,
  SDLK_DELETE, SDLK_END,    SDLK_RETURN,   SDLK_ESCAPE,   SDLK_TAB,   SDLK_F1,
  SDLK_F2,     SDLK_F3,     SDLK_F4,       SDLK_F5,       SDLK_F6,    SDLK_F7,
  SDLK_F8,     SDLK_F9,     SDLK_F10,      SDLK_F11,      SDLK_F12,   SDLK_HOME,
  SDLK_INSERT, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_KP_ENTER,
};

static bool IsUpperLetter(SDL_Keycode key) {
  return key >= SDLK_A && key <= SDLK_Z;
}

static bool IsLowerLetter(SDL_Keycode key) {
  return key >= SDLK_a && key <= SDLK_z;
}

static bool IsProcessableKey(SDL_Keycode key) {
  return key < SDLK_CAPSLOCK || key > SDLK_ENDCALL || specialKeys.contains(key);
}

void InputHandler::HandleKeyboard(const SDL_KeyboardEvent& event) {
  SDL_Scancode scancode = event.scancode;
  SDL_Keymod mod = event.mod;

  if (event.state == SDL_PRESSED) {
    auto modstate = mod;
    if (macOptIsMeta) {
      // remove alt from modstate
      modstate &= ~SDL_KMOD_ALT;
    }
    auto keycode = SDL_GetKeyFromScancode(scancode, modstate);
    if (!IsProcessableKey(keycode)) return;

    std::string keyName = SDL_GetKeyName(keycode);
    if (IsLowerLetter(keycode)) keyName[0] = std::tolower(keyName[0]);
    if (keyName.empty()) return;

    bool modApplied = false;
    std::string inputStr;
    if (mod & SDL_KMOD_CTRL) {
      inputStr += "C-";
      modApplied = true;
    }
    if ((mod & SDL_KMOD_ALT) && macOptIsMeta) {
      inputStr += "M-";
      modApplied = true;
    }
    if (mod & SDL_KMOD_GUI) {
      inputStr += "D-";
      modApplied = true;
    }
    // uppercase ctrl sequence needs S- (due to legacy reasons)
    if ((mod & SDL_KMOD_SHIFT) &&
        (specialKeys.contains(keycode) || (IsUpperLetter(keycode) && (mod & SDL_KMOD_CTRL)))) {
      inputStr += "S-";
      modApplied = true;
    }

    if (!modApplied && keyName == "<") keyName = "<lt>";
    inputStr += keyName;

    if (modApplied) {
      inputStr = "<" + inputStr + ">";
    } else {
      if (specialKeys.contains(keycode)) {
        inputStr = "<" + inputStr + ">";
      }
    }

    // LOG_INFO("Key: {}", inputStr);
    nvim.Input(inputStr);
  }
}

void InputHandler::HandleTextInput(const SDL_TextInputEvent& event) {
  // if (macOptAsAlt && (mod & SDL_KMOD_ALT)) return;

  // std::string inputStr = event.text;
  // if (inputStr == " ") return;
  // if (inputStr == "<") inputStr = "<lt>";

  // // LOG_INFO("Text: {}", inputStr);
  // nvim.Input(inputStr);
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
    case SDL_BUTTON_LEFT:
      buttonStr = "left";
      break;
    case SDL_BUTTON_MIDDLE:
      buttonStr = "middle";
      break;
    case SDL_BUTTON_RIGHT:
      buttonStr = "right";
      break;
    default:
      return;
  }

  std::string actionStr;
  switch (state) {
    case SDL_PRESSED:
      actionStr = "press";
      break;
    case SDL_RELEASED:
      actionStr = "release";
      break;
    case -1:
      actionStr = "drag";
      break;
    default:
      return;
  }

  SDL_Keymod mod = SDL_GetModState();
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
  SDL_Keymod mod = SDL_GetModState();
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
