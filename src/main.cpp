#include <iostream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include "glm/ext/vector_uint2.hpp"

#include "gfx/context.hpp"

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
}

int main() {
  // window ------------------------------------
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  glm::uvec2 size = {1280, 720};
  GLFWwindow *window = glfwCreateWindow(size.x, size.y, "Neovim Gui", NULL, NULL);
  if (!window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }

  glfwSetKeyCallback(window, KeyCallback);

  // webgpu ------------------------------------
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  WGPUContext ctx = WGPUContext(window, {width, height});

  while (!glfwWindowShouldClose(window)) {
    // error callback
    ctx.device.Tick();

    glfwWaitEvents();

    // update -----------------------------------------------------
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
