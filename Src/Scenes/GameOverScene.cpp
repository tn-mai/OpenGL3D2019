/**
* @file GameOverScene.cpp
*/
#include "../GLFWEW.h"
#include "GameOverScene.h"
#include "TitleScene.h"

/**
* �R���X�g���N�^.
*/
GameOverScene::GameOverScene() : Scene("GameOverScene")
{
}

/**
* �V�[��������������.
*
* @retval true  ����������.
* @retval false ���������s. �Q�[���i�s�s�ɂ��A�v���O�������I�����邱��.
*/
bool GameOverScene::Initialize()
{
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");
  SceneFader::Instance().FadeIn(1);
  return true;
}

/**
* �V�[���̓��͂���������.
*/
void GameOverScene::ProcessInput()
{
  if (!SceneFader::Instance().IsFading()) {
    if (isNextScene) {
      SceneStack::Instance().Replace(std::make_shared<TitleScene>());
    } else {
      GLFWEW::Window& window = GLFWEW::Window::Instance();
      const GamePad gamepad = window.GetGamePad();
      if (gamepad.buttonDown & (GamePad::A | GamePad::START)) {
        SceneFader::Instance().FadeOut(1);
        isNextScene = true;
      }
    }
  }
}

/**
* �V�[�����X�V����.
*
* @param deltaTime  �O��̍X�V����̌o�ߎ���(�b).
*/
void GameOverScene::Update(float deltaTime)
{
  fontRenderer.BeginUpdate();
  fontRenderer.Scale(glm::vec2(1));
  fontRenderer.Color(glm::vec4(1));
  fontRenderer.AddString(glm::vec2(-600, 320), L"�Q�[���I�[�o�[���");
  fontRenderer.Scale(glm::vec2(3));
  fontRenderer.Color(glm::vec4(1, 0.2f, 0.1f, 1));
  fontRenderer.AddString(glm::vec2(-288, 0), L"�Q�[���I�[�o�[");
  fontRenderer.EndUpdate();
}

/**
* �V�[����`�悷��.
*/
void GameOverScene::Render()
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();
  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);
}

/**
* �V�[����j������.
*/
void GameOverScene::Finalize()
{
}
