#pragma once

#include "GLFW/glfw3.h"
#include "glm/ext/vector_uint2.hpp"

#include "gfx/context.hpp"

#include <string>
#include <optional>

struct Window {
  static inline WGPUContext _ctx;
  GLFWwindow* window;

  glm::uvec2 size;
  glm::uvec2 fbSize;
  float dpiRatio;

  std::function<void(int, int, int, int)> keyCallback = nullptr;
  std::function<void(unsigned int)> charCallback = nullptr;
  std::function<void(int, int, int)> mouseButtonCallback = nullptr;
  std::function<void(double, double)> cursorPosCallback = nullptr;
  std::function<void(double, double)> scrollCallback = nullptr;
  std::function<void(int, int)> windowSizeCallback = nullptr;
  std::function<void(int, int)> framebufferSizeCallback = nullptr;
  std::function<void(float, float)> windowContentScaleCallback = nullptr;

  Window() = default;
  Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode);
  ~Window();

  void SetShouldClose(bool value);
  bool ShouldClose();
  void SetTitle(const std::string& title);
  void PollEvents();
  void WaitEvents();
  bool KeyPressed(int key);
  bool KeyReleased(int key);
  bool MouseButtonPressed(int button);
  bool MouseButtonReleased(int button);
  glm::vec2 GetCursorPos();
};
