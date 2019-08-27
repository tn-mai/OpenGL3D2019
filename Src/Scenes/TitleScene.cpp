/**
* @file TitleScene.cpp
*/
#include "TitleScene.h"
#include "MainGameScene.h"
#include "../GLFWEW.h"
#include <glm/gtc/matrix_transform.hpp>

/**
* �R���X�g���N�^.
*/
TitleScene::TitleScene() : Scene("TitleScene")
{
}

/**
* �V�[��������������.
*
* @retval true  ����������.
* @retval false ���������s. �Q�[���i�s�s�ɂ��A�v���O�������I�����邱��.
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
* �V�[���̓��͂���������.
*/
void TitleScene::ProcessInput()
{
  // �V�[���؂�ւ�.
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
* �V�[�����X�V����.
*
* @param deltaTime  �O��̍X�V����̌o�ߎ���(�b).
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
  fontRenderer.AddString(glm::vec2(-w * 0.5f + 32, h * 0.5f - lh), L"�^�C�g�����");
  fontRenderer.Scale(glm::vec2(4));
  fontRenderer.AddString(glm::vec2(-300, 0), L"��u��V�s�^");
  fontRenderer.EndUpdate();
}

/**
* �V�[����`�悷��.
*/
void TitleScene::Render()
{
  const GLFWEW::Window& window = GLFWEW::Window::Instance();
  const glm::vec2 screenSize(window.Width(), window.Height());
  spriteRenderer.Draw(screenSize);
  fontRenderer.Draw(screenSize);
}

/**
* �V�[����j������.
*/
void TitleScene::Finalize()
{
  sprites.clear();
}
