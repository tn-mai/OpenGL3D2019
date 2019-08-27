/**
* @file TitleScene.cpp
*/
#include "TitleScene.h"
#include "MainGameScene.h"
#include "../GLFWEW.h"
#include <glm/gtc/matrix_transform.hpp>

/**
* コンストラクタ.
*/
TitleScene::TitleScene() : Scene("TitleScene")
{
}

/**
* シーンを初期化する.
*
* @retval true  初期化成功.
* @retval false 初期化失敗. ゲーム進行不可につき、プログラムを終了すること.
*/
bool TitleScene::Initialize()
{
  spriteRenderer.Init(1000, "Res/Sprite.vert", "Res/Sprite.frag");
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");
  Sprite spr(Texture::Image2D::Create("Res/TitleBg.tga"));
  spr.Scale(glm::vec2(2));
  sprites.push_back(spr);
  SceneFader::Instance().FadeOut(0);
  SceneFader::Instance().FadeIn(1);
  return true;
}

/**
* シーンの入力を処理する.
*/
void TitleScene::ProcessInput()
{
  // シーン切り替え.
  SceneStack& sceneStack = SceneStack::Instance();
  if (!SceneFader::Instance().IsFading()) {
    if (!isNext) {
      GLFWEW::Window& window = GLFWEW::Window::Instance();
      if (window.GetGamePad().buttonDown & (GamePad::A | GamePad::START)) {
        SceneFader::Instance().FadeOut(1);
        isNext = true;
      }
    } else {
      sceneStack.Replace(std::make_shared<MainGameScene>());
    }
  }
}

/**
* シーンを更新する.
*
* @param deltaTime  前回の更新からの経過時間(秒).
*/
void TitleScene::Update(float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const float w = static_cast<float>(window.Width());
  const float h = static_cast<float>(window.Height());
  const float lh = fontRenderer.LineHeight();

  spriteRenderer.BeginUpdate();
  for (const auto& e : sprites) {
    spriteRenderer.AddVertices(e);
  }
  spriteRenderer.EndUpdate();
  fontRenderer.BeginUpdate();
  fontRenderer.Scale(glm::vec2(1));
  fontRenderer.AddString(glm::vec2(-w * 0.5f + 32, h * 0.5f - lh), L"タイトル画面");
  fontRenderer.Scale(glm::vec2(4));
  fontRenderer.AddString(glm::vec2(-300, 0), L"比丘尼遊行録");
  fontRenderer.EndUpdate();
}

/**
* シーンを描画する.
*/
void TitleScene::Render()
{
  const GLFWEW::Window& window = GLFWEW::Window::Instance();
  const glm::vec2 screenSize(window.Width(), window.Height());
  spriteRenderer.Draw(screenSize);
  fontRenderer.Draw(screenSize);
}

/**
* シーンを破棄する.
*/
void TitleScene::Finalize()
{
  sprites.clear();
}
