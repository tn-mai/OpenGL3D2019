/**
* @file Main.cpp
*/
#include <Windows.h>
#include "GLFWEW.h"
#include "Scenes/TitleScene.h"
#include "SkeletalMesh.h"
#include <iostream>

/**
* �v���O�����̃G���g���[�|�C���g.
*/
int main()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (!window.Init(1280, 720, "OpenGL 3D 2019")) {
    return 1;
  }

  Mesh::GlobalSkeletalMeshState::Initialize();

  SceneStack& sceneStack = SceneStack::Instance();
  sceneStack.Push(std::make_shared<TitleScene>());
  sceneStack.Current().Initialize();

  while (!window.ShouldClose()) {
    window.Update();
    if (window.KeyDown(GLFW_KEY_ESCAPE)) {
      if (MessageBox(nullptr, "�Q�[�����I�����܂����H", "�I��", MB_YESNO) == IDYES) {
        break;
      }
    }

    Mesh::GlobalSkeletalMeshState::ResetUniformData();

    sceneStack.Update(static_cast<float>(window.DeltaTime()));

    Mesh::GlobalSkeletalMeshState::UploadUniformData();

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    sceneStack.Render();

    window.SwapBuffers();
  }

  Mesh::GlobalSkeletalMeshState::Finalize();
}