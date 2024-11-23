#include "./input.hpp"

#include "editor/window.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <set>
#include <utility>

InputHandler::InputHandler(
  Nvim* nvim,
  WinManager* winManager,
  const Options& options
)
  : nvim(nvim), winManager(winManager), options(options) {
}

static const std::set<SDL_Keycode> specialKeys{
  SDLK_SPACE,  SDLK_UP,     SDLK_DOWN,     SDLK_LEFT,     SDLK_RIGHT, SDLK_BACKSPACE,
  SDLK_DELETE, SDLK_END,    SDLK_RETURN,   SDLK_ESCAPE,   SDLK_TAB,   SDLK_F1,
  SDLK_F2,     SDLK_F3,     SDLK_F4,       SDLK_F5,       SDLK_F6,    SDLK_F7,
  SDLK_F8,     SDLK_F9,     SDLK_F10,      SDLK_F11,      SDLK_F12,   SDLK_HOME,
  SDLK_INSERT, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_KP_ENTER,
};

static bool IsUpperLetter(SDL_Keycode key) {
  return key >= 65 && key <= 90;
}

static bool IsLowerLetter(SDL_Keycode key) {
  return key >= 97 && key <= 122;
}

static bool IsProcessableKey(SDL_Keycode key) {
  return key < SDLK_CAPSLOCK || key > SDLK_ENDCALL || specialKeys.contains(key);
}

// static void PrintMods(SDL_Keymod mod) {
//   std::string modStr;
//   if (mod & SDL_KMOD_LSHIFT) modStr += "LShift ";
//   if (mod & SDL_KMOD_RSHIFT) modStr += "RShift ";
//   if (mod & SDL_KMOD_LCTRL) modStr += "LCtrl ";
//   if (mod & SDL_KMOD_RCTRL) modStr += "RCtrl ";
//   if (mod & SDL_KMOD_LALT) modStr += "LAlt ";
//   if (mod & SDL_KMOD_RALT) modStr += "RAlt ";
//   if (mod & SDL_KMOD_LGUI) modStr += "LGui ";
//   if (mod & SDL_KMOD_RGUI) modStr += "RGui ";
//   if (mod & SDL_KMOD_NUM) modStr += "Num ";
//   if (mod & SDL_KMOD_CAPS) modStr += "Caps ";
//   if (mod & SDL_KMOD_MODE) modStr += "Mode ";
//   LOG_INFO("mods: {}", modStr);
// }

void InputHandler::HandleKeyboard(const SDL_KeyboardEvent& event) {
  SDL_Scancode scancode = event.scancode;
  SDL_Keymod mod = event.mod;
  // PrintMods(mod);

  if (event.down) {
    auto modstate = mod;
    if (options.macOptIsMeta) {
      // remove alt from modstate
      modstate &= ~SDL_KMOD_ALT;
    }
    auto keycode = SDL_GetKeyFromScancode(scancode, modstate, false);
    if (!IsProcessableKey(keycode)) return;

    std::string keyName = SDL_GetKeyName(keycode);
    if (keyName.empty()) return;
    if (IsLowerLetter(keycode)) keyName[0] = std::tolower(keyName[0]);

    bool modApplied = false;
    std::string inputStr;
    if (mod & SDL_KMOD_CTRL) {
      inputStr += "C-";
      modApplied = true;
    }
    if ((mod & SDL_KMOD_ALT) && options.macOptIsMeta) {
      inputStr += "M-";
      modApplied = true;
    }
    if (mod & SDL_KMOD_GUI) {
      inputStr += "D-";
      modApplied = true;
    }
    // uppercase ctrl sequence needs S- (due to legacy reasons)
    if ((mod & SDL_KMOD_SHIFT) && (specialKeys.contains(keycode) ||
                                   (IsUpperLetter(keycode) && (mod & SDL_KMOD_CTRL)))) {
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
    nvim->Input(inputStr);
  }
}

void InputHandler::HandleTextEditing(const SDL_TextEditingEvent& event) {

}

void InputHandler::HandleTextInput(const SDL_TextInputEvent& event) {
  std::string inputStr = event.text;
  if (inputStr == " ") return;
  if (inputStr == "<") inputStr = "<lt>";

  // LOG_INFO("Text: {}", inputStr);
  // nvim->Input(inputStr);
}

void InputHandler::HandleMouseButton(const SDL_MouseButtonEvent& event) {
  if (event.down) {
    if (event.y < options.marginTop) return;

    mouseButton = event.button;
    HandleMouseButtonAndMotion(event.down, {event.x, event.y});

  } else {
    HandleMouseButtonAndMotion(event.down, {event.x, event.y});
    currGrid.reset();
    mouseButton.reset();
  }
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
    case 0:
      actionStr = "release";
      break;
    case 1:
      actionStr = "press";
      break;
    case -1:
      actionStr = "drag";
      break;
    default:
      std::unreachable();
  }

  SDL_Keymod mod = SDL_GetModState();
  std::string modStr;
  if (mod & SDL_KMOD_CTRL) modStr += "C-";
  if (mod & SDL_KMOD_ALT) modStr += "M-";
  if (mod & SDL_KMOD_GUI) modStr += "D-";
  if (mod & SDL_KMOD_SHIFT) modStr += "S-";

  MouseInfo info;
  if (!currGrid.has_value()) {
    info = winManager->GetMouseInfo(mousePos);
    currGrid = info.grid;
  } else {
    info = winManager->GetMouseInfo(*currGrid, mousePos);
  }
  if (!Options::multigrid) info.grid = 0;

  // LOG_INFO(
  //   "buttonStr {}, actionStr {}, modStr {}, info.grid {}, info.row {}, info.col {}",
  //   buttonStr, actionStr, modStr, info.grid, info.row, info.col
  // );
  nvim->InputMouse(buttonStr, actionStr, modStr, info.grid, info.row, info.col);
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
    info = winManager->GetMouseInfo(mousePos);
  } else {
    info = winManager->GetMouseInfo(*currGrid, mousePos);
  }
  if (!options.multigrid) info.grid = 0;

  double scrollUnit = 1 / options.scrollSpeed;

  double yDelta = event.y;
  double xDelta = event.x;
  double yAbs = std::abs(event.y);
  double xAbs = std::abs(event.x);

  // only scroll one axis at a time
  if (yAbs > xAbs) {
    yAccum += yDelta;
    yAccum = std::min(yAccum, 100.0);
    yAccum = std::max(yAccum, -100.0);

    while (yAccum >= scrollUnit) {
      nvim->InputMouse("wheel", "up", modStr, info.grid, info.row, info.col);
      yAccum -= scrollUnit;
    }
    while (yAccum < 0) {
      nvim->InputMouse("wheel", "down", modStr, info.grid, info.row, info.col);
      yAccum += scrollUnit;
    }

  } else if (xAbs > yAbs) {
    xAccum += xDelta;
    xAccum = std::min(xAccum, 100.0);
    xAccum = std::max(xAccum, -100.0);

    while (xAccum >= scrollUnit) {
      nvim->InputMouse("wheel", "right", modStr, info.grid, info.row, info.col);
      xAccum -= scrollUnit;
    }
    while (xAccum < 0) {
      nvim->InputMouse("wheel", "left", modStr, info.grid, info.row, info.col);
      xAccum += scrollUnit;
    }
  }
}
