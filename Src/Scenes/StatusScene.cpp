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
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");
  return true;
}

/**
* シーンの入力を処理する.
*/
void StatusScene::ProcessInput()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const GamePad gamepad = window.GetGamePad();
  if (gamepad.buttonDown & GamePad::B) {
    SceneStack::Instance().Pop();
  }
}

/**
* シーンを更新する.
*
* @param deltaTime  前回の更新からの経過時間(秒).
*/
void StatusScene::Update(float deltaTime)
{
  fontRenderer.BeginUpdate();
  fontRenderer.AddString(glm::vec2(-600, 320), L"ステータス画面");
  fontRenderer.AddString(glm::vec2(-400, 360 - 32 * 4), L"体力：　２０／２２");
  fontRenderer.AddString(glm::vec2(-400, 360 - 32 * 6), L"法力：　１５／１５");
  fontRenderer.AddString(glm::vec2(-400, 360 - 32 * 8), L"武器：　銅の錫杖");
  fontRenderer.AddString(glm::vec2(-400, 360 - 32 * 10), L"防具：　古びた裳付け衣");
  fontRenderer.EndUpdate();
}

/**
* シーンを描画する.
*/
void StatusScene::Render()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);
}

/**
* シーンを破棄する.
*/
void StatusScene::Finalize()
{
}

/**
* シーンを活動状態にする.
*/
void StatusScene::Play()
{
  GLFWEW::Window::Instance().EnableMouseCursor();
  Scene::Play();
}
