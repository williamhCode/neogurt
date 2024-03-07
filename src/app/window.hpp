#pragma once

#include "GLFW/glfw3.h"
#include "glm/ext/vector_uint2.hpp"

#include "gfx/context.hpp"

#include <functional>
#include <optional>
#include <string>

struct Window {
  static inline WGPUContext _ctx;
  GLFWwindow* window;
  glm::uvec2 size;

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
  using Event = std::variant<KeyData, CharData, MouseButtonData, CursorPosData>;
  std::vector<Event> events;
  struct WindowSizeData {
    int width;
    int height;
  };
  // std::optional<WindowSizeData> windowSizeEvent;

  // void (*keyCallback)(int, int, int, int) = nullptr;
  // void (*charCallback)(unsigned int) = nullptr;
  // void (*mouseButtonCallback)(int, int, int) = nullptr;
  // void (*cursorPosCallback)(double, double) = nullptr;
  // void (*windowSizeCallback)(int, int) = nullptr;
  std::function<void(int, int, int, int)> keyCallback = nullptr;
  std::function<void(unsigned int)> charCallback = nullptr;
  std::function<void(int, int, int)> mouseButtonCallback = nullptr;
  std::function<void(double, double)> cursorPosCallback = nullptr;
  std::function<void(int, int)> windowSizeCallback = nullptr;

  std::mutex renderMutex;

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
