/**
* @file Main.cpp
*/
#include "GLFWEW.h"
#include <iostream>

int main()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (!window.Init(1280, 720, "OpenGL 3D 2019")) {
    return 1;
  }

  while (!window.ShouldClose()) {
    window.Update();

    if (window.KeyDown(GLFW_KEY_ESCAPE)) {
      break;
    }

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    window.SwapBuffers();
  }
}