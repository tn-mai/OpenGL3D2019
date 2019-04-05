/**
* @file MainGameScene.cpp
*/
#include "MainGameScene.h"
#include "StatusScene.h"
#include "GameOverScene.h"
#include "../GLFWEW.h"

/**
* コンストラクタ.
*/
MainGameScene::MainGameScene() : Scene("MainGameScene")
{
}

/**
* シーンを初期化する.
*
* @retval true  初期化成功.
* @retval false 初期化失敗. ゲーム進行不可につき、プログラムを終了すること.
*/
bool MainGameScene::Initialize()
{
  return true;
}

/**
* シーンを更新する.
*
* @param sceneStack シーン制御オブジェクト.
* @param deltaTime  前回の更新からの経過時間(秒).
*/

void MainGameScene::Update(SceneStack& sceneStack, float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (window.KeyDown(GLFW_KEY_A)) {
    sceneStack.Replace(std::make_shared<StatusScene>());
  } else if (window.KeyDown(GLFW_KEY_ENTER)) {
    sceneStack.Replace(std::make_shared<GameOverScene>());
  }
}

/**
* シーンを描画する.
*/
void MainGameScene::Render()
{
}

/**
* シーンを破棄する.
*/
void MainGameScene::Finalize()
{
}
