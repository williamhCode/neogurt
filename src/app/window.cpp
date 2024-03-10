#include "window.hpp"
#include "utils/logger.hpp"

#include <iostream>
#include <ostream>

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.keyCallback) win.keyCallback(key, scancode, action, mods);
}

void CharCallback(GLFWwindow* window, unsigned int codepoint) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.charCallback) win.charCallback(codepoint);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.mouseButtonCallback) win.mouseButtonCallback(button, action, mods);
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.cursorPosCallback) win.cursorPosCallback(xpos, ypos);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.scrollCallback) win.scrollCallback(xoffset, yoffset);
}

void WindowSizeCallback(GLFWwindow* window, int width, int height) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  win.size = {width, height};
  if (win.windowSizeCallback) win.windowSizeCallback(width, height);
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  Window& win = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  win.fbSize = {width, height};
  if (win.framebufferSizeCallback) win.framebufferSizeCallback(width, height);
}

Window::Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode)
    : size(size) {
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
  glfwSetCharCallback(window, CharCallback);
  glfwSetMouseButtonCallback(window, MouseButtonCallback);
  glfwSetCursorPosCallback(window, CursorPosCallback);
  glfwSetScrollCallback(window, ScrollCallback);
  glfwSetWindowSizeCallback(window, WindowSizeCallback);
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  fbSize = {width, height};

  // webgpu ------------------------------------
  _ctx = WGPUContext(window, fbSize, presentMode);
  std::cout << "WGPUContext created" << std::endl;
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

void Window::SetTitle(const std::string& title) {
  glfwSetWindowTitle(window, title.c_str());
}

void Window::PollEvents() {
  glfwPollEvents();
}

void Window::WaitEvents() {
  glfwWaitEvents();
}

bool Window::KeyPressed(int key) {
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool Window::KeyReleased(int key) {
  return glfwGetKey(window, key) == GLFW_RELEASE;
}

bool Window::MouseButtonPressed(int button) {
  return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

bool Window::MouseButtonReleased(int button) {
  return glfwGetMouseButton(window, button) == GLFW_RELEASE;
}

glm::vec2 Window::GetCursorPos() {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  return {xpos, ypos};
}
