/**
* @file TitleScene.cpp
*/
#include "TitleScene.h"
#include "MainGameScene.h"
#include "../GLFWEW.h"

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
  Sprite spr(std::make_shared<Texture::Image2D>("Res/Title.tga"));
  sprites.push_back(spr);
  return true;
}

/**
* シーンを更新する.
*
* @param sceneStack シーン制御オブジェクト.
* @param deltaTime  前回の更新からの経過時間(秒).
*/
void TitleScene::Update(SceneStack& sceneStack, float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  if (window.KeyDown(GLFW_KEY_ENTER)) {
    sceneStack.Replace(std::make_shared<MainGameScene>());
  }
  spriteRenderer.BeginUpdate();
  for (const auto& e : sprites) {
    spriteRenderer.AddVertices(e);
  }
  spriteRenderer.EndUpdate();
  fontRenderer.BeginUpdate();
  fontRenderer.AddString(glm::vec2(0, 0), L"タイトル画面");
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
