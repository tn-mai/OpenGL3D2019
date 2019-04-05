/**
* @file Main.cpp
*/
#include <Windows.h>
#include "GLFWEW.h"
#include "Scenes/TitleScene.h"
#include <iostream>

/**
* プログラムのエントリーポイント.
*/
int main()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (!window.Init(1280, 720, "OpenGL 3D 2019")) {
    return 1;
  }

  SceneStack sceneStack;
  sceneStack.Push(std::make_shared<TitleScene>());

  while (!window.ShouldClose()) {
    window.Update();
    if (window.KeyDown(GLFW_KEY_ESCAPE)) {
      if (MessageBox(nullptr, "ゲームを終了しますか？", "終了", MB_YESNO) == IDYES) {
        break;
      }
    }

    sceneStack.Update(static_cast<float>(window.DeltaTime()));

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sceneStack.Render();

    window.SwapBuffers();
  }
}