#pragma once

#include "SDL3/SDL_events.h"

#include "nvim/nvim.hpp"
#include "editor/window.hpp"

#include <optional>
#include <atomic>

enum class MacosOptionIsMeta {
  None,
  OnlyLeft,
  OnlyRight,
  Both,
};

inline MacosOptionIsMeta ParseMacosOptionIsMeta(const std::string& str) {
  if (str == "only_left") return MacosOptionIsMeta::OnlyLeft;
  if (str == "only_right") return MacosOptionIsMeta::OnlyRight;
  if (str == "both") return MacosOptionIsMeta::Both;
  return MacosOptionIsMeta::None;
}

struct InputHandler {
  WinManager* winManager;
  Nvim* nvim;

  inline static bool multigrid;
  inline static std::atomic<MacosOptionIsMeta> macosOptionIsMeta;
  inline static std::atomic<float> scrollSpeed;
  inline static std::atomic_int titlebarHeight;

  // ime
  std::string editingText;

  // mouse related
  std::optional<int> mouseButton;
  std::optional<int> currGrid;

  double yAccum = 0;
  double xAccum = 0;

  void HandleKeyboard(const SDL_KeyboardEvent& event);
  void HandleTextEditing(const SDL_TextEditingEvent& event);
  void HandleTextInput(const SDL_TextInputEvent& event);
  void HandleMouseButton(const SDL_MouseButtonEvent& event);
  void HandleMouseMotion(const SDL_MouseMotionEvent& event);
  void HandleMouseButtonAndMotion(int state, glm::vec2 pos);
  void HandleMouseWheel(const SDL_MouseWheelEvent& event);
};
