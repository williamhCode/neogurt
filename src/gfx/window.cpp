#include "window.hpp"

#include <iostream>
#include <ostream>

const WGPUContext &ctx = Window::ctx;

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  Window *win = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  win->keyCallbacks.push_back({key, scancode, action, mods});
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  Window *win = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  win->mouseButtonCallbacks.push_back({button, action, mods});
}

static void CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  Window *win = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  win->cursorPosCallbacks.push_back({xpos, ypos});
}

Window::Window(glm::uvec2 size, std::string title, wgpu::PresentMode presentMode) {
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(size.x, size.y, title.c_str(), NULL, NULL);
  if (!window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetMouseButtonCallback(window, MouseButtonCallback);

  // webgpu ------------------------------------
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  ctx = WGPUContext(window, {width, height}, presentMode);
}

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::SetShouldClose(bool value) {
  glfwSetWindowShouldClose(window, value);
}

bool Window::ShouldClose() {
  return glfwWindowShouldClose(window);
}

void Window::PollEvents() {
  keyCallbacks.clear();
  mouseButtonCallbacks.clear();
  cursorPosCallbacks.clear();
  glfwPollEvents();
}

bool Window::KeyPressed(int key) {
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::KeyReleased(int key) {
  return glfwGetKey(window, key) == GLFW_RELEASE;
}
