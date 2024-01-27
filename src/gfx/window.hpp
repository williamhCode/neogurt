#pragma once

#include "GLFW/glfw3.h"
#include "glm/ext/vector_uint2.hpp"

#include "gfx/context.hpp"

#include <string>

struct Window {
  inline static WGPUContext ctx;
  GLFWwindow* window;

  glm::uvec2 size;

  // callback data
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
  std::vector<KeyData> keyCallbacks;
  std::vector<CharData> charCallbacks;
  std::vector<MouseButtonData> mouseButtonCallbacks;
  std::vector<CursorPosData> cursorPosCallbacks;

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
