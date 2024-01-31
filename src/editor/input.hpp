#pragma once

#include "GLFW/glfw3.h"
#include "utf8/unchecked.h"
#include <unordered_map>
#include <iostream>

inline std::string UnicodeToUTF8(unsigned int unicode) {
  std::string utf8String;
  utf8::unchecked::append(unicode, std::back_inserter(utf8String));
  return utf8String;
}

// clang-format off
static std::unordered_map<int, std::string> specialKeys{
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

// handle when modifier or special keys are pressed.
// returning "" means ignore output
inline std::string KeyInput(int key, int mods) {
  bool isSpecial =
    (key >= GLFW_KEY_ESCAPE && key < GLFW_KEY_LEFT_SHIFT) || key == GLFW_KEY_SPACE;
  bool onlyMod = key >= GLFW_KEY_LEFT_SHIFT && key <= GLFW_KEY_RIGHT_SUPER;
  bool onlyShift = (mods ^ GLFW_MOD_SHIFT) == 0;
  if ((!mods && !isSpecial) || onlyMod || (onlyShift && !isSpecial)) return "";

  auto keyName = isSpecial ? specialKeys[key] : glfwGetKeyName(key, 0);
  if (keyName == "") return "";

  std::string out = "<";
  if (mods & GLFW_MOD_CONTROL) out += "C-";
  if (mods & GLFW_MOD_ALT) out += "M-";
  if (mods & GLFW_MOD_SUPER) out += "D-";
  if (mods & GLFW_MOD_SHIFT) out += "S-";
  return out + keyName + ">";
}

// returning "" means ignore output
inline std::string CharInput(unsigned int unicode) {
  // handle space in KeyInput
  if (unicode == GLFW_KEY_SPACE) return "";
  return UnicodeToUTF8(unicode);
}
