#include "window.hpp"

#include <iostream>
#include <ostream>

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.keyCallback) win.keyCallback(key, scancode, action, mods);
}

static void CharCallback(GLFWwindow* window, unsigned int codepoint) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.charCallback) win.charCallback(codepoint);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.mouseButtonCallback) win.mouseButtonCallback(button, action, mods);
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.cursorPosCallback) win.cursorPosCallback(xpos, ypos);
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.scrollCallback) win.scrollCallback(xoffset, yoffset);
}

static void WindowSizeCallback(GLFWwindow* window, int width, int height) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  win.size = {width, height};
  if (win.windowSizeCallback) win.windowSizeCallback(width, height);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  win.fbSize = {width, height};
  if (win.framebufferSizeCallback) win.framebufferSizeCallback(width, height);
}

static void WindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  win.dpiScale = xscale;
  if (win.windowContentScaleCallback) win.windowContentScaleCallback(xscale, yscale);
}

static void WindowCloseCallback(GLFWwindow* window) {
  AppWindow& win = *reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
  if (win.windowCloseCallback) win.windowCloseCallback();
}

AppWindow::AppWindow(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode)
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
  glfwSetWindowContentScaleCallback(window, WindowContentScaleCallback);
  glfwSetWindowCloseCallback(window, WindowCloseCallback);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  fbSize = {width, height};

  float xscale, yscale;
  glfwGetWindowContentScale(window, &xscale, &yscale);
  dpiScale = xscale;

  // webgpu ------------------------------------
  _ctx = WGPUContext(window, fbSize, presentMode);
  std::cout << "WGPUContext created" << std::endl;
}

AppWindow::~AppWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void AppWindow::SetShouldClose(bool value) {
  glfwSetWindowShouldClose(window, value);
}

bool AppWindow::ShouldClose() {
  return glfwWindowShouldClose(window);
}

void AppWindow::SetTitle(const std::string& title) {
  glfwSetWindowTitle(window, title.c_str());
}

void AppWindow::PollEvents() {
  glfwPollEvents();
}

void AppWindow::WaitEvents() {
  glfwWaitEvents();
}

bool AppWindow::KeyPressed(int key) {
  return glfwGetKey(window, key) == GLFW_PRESS;
}

bool AppWindow::KeyReleased(int key) {
  return glfwGetKey(window, key) == GLFW_RELEASE;
}

bool AppWindow::MouseButtonPressed(int button) {
  return glfwGetMouseButton(window, button) == GLFW_PRESS;
}

bool AppWindow::MouseButtonReleased(int button) {
  return glfwGetMouseButton(window, button) == GLFW_RELEASE;
}

glm::vec2 AppWindow::GetCursorPos() {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  return {xpos, ypos};
}
