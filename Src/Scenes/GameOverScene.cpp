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
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");
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
  fontRenderer.BeginUpdate();
  fontRenderer.Scale(glm::vec2(1));
  fontRenderer.Color(glm::vec4(1));
  fontRenderer.AddString(glm::vec2(-600, 320), L"ゲームオーバー画面");
  fontRenderer.Scale(glm::vec2(3));
  fontRenderer.Color(glm::vec4(1, 0.2f, 0.1f, 1));
  fontRenderer.AddString(glm::vec2(-288, 0), L"ゲームオーバー");
  fontRenderer.EndUpdate();

  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const GamePad gamepad = window.GetGamePad();
  if (gamepad.buttonDown & (GamePad::A | GamePad::START)) {
    sceneStack.Replace(std::make_shared<TitleScene>());
  }
}
/**
* シーンを描画する.
*/
void GameOverScene::Render()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);
}

/**
* シーンを破棄する.
*/
void GameOverScene::Finalize()
{
}
