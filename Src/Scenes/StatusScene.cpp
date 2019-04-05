/**
* @file StatusScene.cpp
*/
#include "StatusScene.h"
#include "../GLFWEW.h"

/**
* コンストラクタ.
*/
StatusScene::StatusScene() : Scene("StatusScene")
{
}

/**
* シーンを初期化する.
*
* @retval true  初期化成功.
* @retval false 初期化失敗. ゲーム進行不可につき、プログラムを終了すること.
*/
bool StatusScene::Initialize()
{
  return true;
}

/**
* シーンを更新する.
*
* @param sceneStack シーン制御オブジェクト.
* @param deltaTime  前回の更新からの経過時間(秒).
*/
void StatusScene::Update(SceneStack& sceneStack, float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (window.KeyDown(GLFW_KEY_ENTER)) {
    sceneStack.Pop();
  }
}

/**
* シーンを描画する.
*/
void StatusScene::Render()
{
}

/**
* シーンを破棄する.
*/
void StatusScene::Finalize()
{
}

