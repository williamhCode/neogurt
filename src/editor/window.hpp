#pragma once

#include "GLFW/glfw3.h"
#include "glm/ext/vector_uint2.hpp"

#include "gfx/context.hpp"

#include <string>

struct Window {
  static inline WGPUContext _ctx;
  GLFWwindow* window;
  glm::uvec2 size;

  enum class EventType {
    Key,
    Char,
    MouseButton,
    CursorPos,
  };
  struct KeyData {
    int key;
    int scancode;
    int action;
    int mods;
  };
  struct CharData {
    unsigned int codepoint;
  };
  struct MouseButtonData {
    int button;
    int action;
    int mods;
  };
  struct CursorPosData {
    double xpos;
    double ypos;
  };
  using EventData = std::variant<KeyData, CharData, MouseButtonData, CursorPosData>;
  struct Event {
    EventType type;
    EventData data;
  };
  std::vector<Event> events;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode);
  ~Window();

  void SetShouldClose(bool value);
  bool ShouldClose();

  void PollEvents();
  void WaitEvents();
  bool KeyPressed(int key);
  bool KeyReleased(int key);
};
