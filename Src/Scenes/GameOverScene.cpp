/**
* @file GameOverScene.cpp
*/
#include "../GLFWEW.h"
#include "GameOverScene.h"
#include "TitleScene.h"

/**
* コンストラクタ.
*/
GameOverScene::GameOverScene() : Scene("GameOverScene")
{
}

/**
* シーンを初期化する.
*
* @retval true  初期化成功.
* @retval false 初期化失敗. ゲーム進行不可につき、プログラムを終了すること.
*/
bool GameOverScene::Initialize()
{
  return true;
}

/**
* シーンを更新する.
*
* @param sceneStack シーン制御オブジェクト.
* @param deltaTime  前回の更新からの経過時間(秒).
*/
void GameOverScene::Update(SceneStack& sceneStack, float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (window.KeyDown(GLFW_KEY_ENTER)) {
    sceneStack.Replace(std::make_shared<TitleScene>());
  }
}
/**
* シーンを描画する.
*/
void GameOverScene::Render()
{
}

/**
* シーンを破棄する.
*/
void GameOverScene::Finalize()
{
}
