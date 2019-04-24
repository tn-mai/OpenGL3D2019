/**
* @file MainGameScene.cpp
*/
#include "MainGameScene.h"
#include "StatusScene.h"
#include "GameOverScene.h"
#include "../GLFWEW.h"
#include <glm/gtc/matrix_transform.hpp>

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
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");

  meshBuffer.Init(sizeof(Mesh::Vertex) * 1000000, sizeof(GLushort) * 3000000);
  meshBuffer.CreateCircle("Circle", 8);
  heightMap.Load("Res/HeightMap.tga", 100.0f, 0.5f);
  heightMap.CreateMesh(meshBuffer, "Terrain");
  texTerrain = std::make_shared<Texture::Image2D>("Res/ColorMap.tga");

  progMesh = Shader::Cache::Instance().Create("Res/Mesh.vert", "Res/Mesh.frag");
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

  fontRenderer.BeginUpdate();
  if (IsActive()) {
    fontRenderer.AddString(glm::vec2(-600, 320), L"メインゲーム画面");
  }
  fontRenderer.EndUpdate();

  if (IsActive()) {
    const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
    const float dt = static_cast<float>(window.DeltaTime());
    if (window.KeyPressed(GLFW_KEY_W)) {
      pos += dir * dt * 10.0f;
    } else if (window.KeyPressed(GLFW_KEY_S)) {
      pos -= dir * dt * 10.0f;
    }
    if (window.KeyPressed(GLFW_KEY_A)) {
      pos += left * dt * 10.0f;
    } else if (window.KeyPressed(GLFW_KEY_D)) {
      pos -= left * dt * 10.0f;
    }

    const glm::vec2 currentMousePos = GLFWEW::Window::Instance().MousePosition();
    const glm::vec2 mouseMove = currentMousePos - prevMousePos;
    prevMousePos = currentMousePos;
    glm::mat4 matRX(1);
    if (mouseMove.x) {
      matRX = glm::rotate(glm::mat4(1), -mouseMove.x / 100.0f, glm::vec3(0, 1, 0));
    }
    glm::mat4 matRY(1);
    if (mouseMove.y) {
      matRY = glm::rotate(glm::mat4(1), mouseMove.y / 100.0f, left);
    }
    dir = matRX * matRY * glm::vec4(dir, 1);
    dir = normalize(dir);

    // シーン切り替え.
    if (window.KeyDown(GLFW_KEY_SPACE)) {
      sceneStack.Push(std::make_shared<StatusScene>());
    } else if (window.KeyDown(GLFW_KEY_ENTER)) {
      sceneStack.Replace(std::make_shared<GameOverScene>());
    }
  }
}

/**
* シーンを描画する.
*/
void MainGameScene::Render()
{
  const GLFWEW::Window& window = GLFWEW::Window::Instance();

  meshBuffer.Bind();
  progMesh->Use();

  const glm::mat4 matView = glm::lookAt(pos, pos + dir, glm::vec3(0, 1, 0));
  const float aspectRatio = static_cast<float>(window.Width()) / static_cast<float>(window.Height());
  const glm::mat4 matProj = glm::perspective(glm::radians(30.0f), aspectRatio, 1.0f, 1000.0f);
  const glm::mat4 matModel = glm::scale(glm::mat4(1), glm::vec3(1));
  progMesh->SetViewProjectionMatrix(matProj * matView * matModel);

  texTerrain->Bind(0);
  meshBuffer.GetMesh("Terrain").Draw();
  meshBuffer.GetMesh("Circle").Draw();
  texTerrain->Unbind(0);

  progMesh->Unuse();
  meshBuffer.Unbind();

  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);
}

/**
* シーンを破棄する.
*/
void MainGameScene::Finalize()
{
  GLFWEW::Window::Instance().EnableMouseCursor();
}

/**
* シーンを活動状態にする.
*/
void MainGameScene::Play()
{
  GLFWEW::Window::Instance().DisableMouseCursor();
  prevMousePos = GLFWEW::Window::Instance().MousePosition();
  Scene::Play();
}
