#include "window.hpp"
#include "utils/logger.hpp"

static void
KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.keyCallback) win.keyCallback(key, scancode, action, mods);
}

static void CharCallback(GLFWwindow* window, unsigned int codepoint) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.charCallback) win.charCallback(codepoint);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.mouseButtonCallback) win.mouseButtonCallback(button, action, mods);
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.cursorPosCallback) win.cursorPosCallback(xpos, ypos);
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.scrollCallback) win.scrollCallback(xoffset, yoffset);
}

static void WindowSizeCallback(GLFWwindow* window, int width, int height) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  win.size = {width, height};
  if (win.windowSizeCallback) win.windowSizeCallback(width, height);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  win.fbSize = {width, height};
  if (win.framebufferSizeCallback) win.framebufferSizeCallback(width, height);
}

static void WindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  win.dpiScale = xscale;
  if (win.windowContentScaleCallback) win.windowContentScaleCallback(xscale, yscale);
}

static void WindowCloseCallback(GLFWwindow* window) {
  Window& win = *static_cast<Window*>(glfwGetWindowUserPointer(window));
  if (win.windowCloseCallback) win.windowCloseCallback();
}

Window::Window(glm::uvec2 size, const std::string& title, wgpu::PresentMode presentMode)
    : size(size) {
  if (!glfwInit()) {
    LOG_ERR("Could not initialize GLFW!");
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

  window = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
  if (!window) {
    LOG_ERR("Could not open window!");
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
  LOG_INFO("WGPUContext created");

  if (glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER)) {
    LOG_INFO("GLFW_TRANSPARENT_FRAMEBUFFER is supported");
  }
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
