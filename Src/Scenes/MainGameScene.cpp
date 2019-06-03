/**
* @file MainGameScene.cpp
*/
#include "MainGameScene.h"
#include "StatusScene.h"
#include "GameOverScene.h"
#include "../GLFWEW.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <sstream>
#include <iomanip>

/*
  メッセージモードの開始 begin ラベル
  msg テキスト
  select 選択肢
  set 変数番号 数値or変数番号
  add 変数番号 数値or変数番号
  sub 変数番号 数値or変数番号
  if 変数番号(が0でなければ) ジャンプ先ラベル
  メッセージモードの終了 end

  今後の講義予定:
  - メッシュ表示
  - ハイトマップ
  - glTFメッシュ表示(ボーナストラック:　アニメーション)
  - 球と球の衝突判定
  - 矩形と球の衝突判定
  - イベント処理(会話、フラグ制御)
*/

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

  meshBuffer.Init(sizeof(Mesh::Vertex) * 1000000, sizeof(GLushort) * 3000000, sizeof(Mesh::UniformDataMeshMatrix) * 100);
  meshBuffer.CreateCircle("Circle", 8);
  heightMap.Load("Res/HeightMap.tga", 100.0f, 0.5f);
  heightMap.CreateMesh(meshBuffer, "Terrain");
  texTerrain = Texture::Image2D::Create("Res/ColorMap.tga");
  texTree = Texture::Image2D::Create("Res/TestTree.tga");
  texOniSmall = Texture::Image2D::Create("Res/oni_s_albedo.tga");
  meshBuffer.LoadMesh("Res/bikuni.gltf");
  meshBuffer.LoadMesh("Res/TestTree.gltf");
  meshBuffer.LoadMesh("Res/oni_small.gltf");
  meshPlayer = meshBuffer.GetMesh("Bikuni");
  meshPlayer->SetAnimation(0);
  meshTerrain = meshBuffer.GetMesh("Terrain");
  meshCircle = meshBuffer.GetMesh("Circle");

  glm::vec3 startPos(100, 0, 150);
  startPos.y = heightMap.Height(startPos);
  meshPlayer->translation = startPos;

  static const size_t treeCount = 200;
  meshTrees.reserve(treeCount);
  std::mt19937 rand;
  rand.seed(0);
  for (size_t i = 0; i < treeCount; ++i) {
    Mesh::MeshPtr p = meshBuffer.GetMesh("TestTree");
    p->translation.x = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().x - 2)(rand));
    p->translation.z = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().y - 2)(rand));
    p->translation.y = heightMap.Height(p->translation);
    p->rotation = glm::angleAxis(std::uniform_real_distribution<float>(0, glm::half_pi<float>())(rand), glm::vec3(0, 1, 0));
    p->scale = glm::vec3(std::normal_distribution<float>(0.7f, 0.2f)(rand));
    p->scale = glm::clamp(p->scale, 0.4f, 1.2f);
    p->SetAnimation(0);
    p->frame = std::uniform_real_distribution<float>(0, 2)(rand);
    meshTrees.push_back(p);
  }

  static const size_t oniCount = 10;
  meshEnemies.reserve(oniCount);
  for (size_t i = 0; i < oniCount; ++i) {
    Mesh::MeshPtr p = meshBuffer.GetMesh("oni_small");
    p->translation.x = static_cast<float>(std::uniform_int_distribution<>(-5, 5)(rand)) + startPos.x;
    p->translation.z = static_cast<float>(std::uniform_int_distribution<>(-5, 5)(rand)) + startPos.z;
    p->translation.y = heightMap.Height(p->translation);
    p->rotation = glm::angleAxis(std::uniform_real_distribution<float>(0, glm::half_pi<float>() * 4)(rand), glm::vec3(0, 1, 0));
    //p->scale = glm::vec3(std::normal_distribution<float>(0.0f, 1.0f)(rand));
    //p->scale = glm::clamp(p->scale, 0.8f, 1.2f);
    p->SetAnimation(2);
    meshEnemies.push_back(p);
  }

  Shader::Cache& shaderCache = Shader::Cache::Instance();
  progMesh = shaderCache.Create("Res/Mesh.vert", "Res/Mesh.frag");
  progSkeletalMesh = shaderCache.Create("Res/SkeletalMesh.vert", "Res/SkeletalMesh.frag");
  progSkeletalMesh->BindUniformBlock("MeshMatrixUniformData", 0);

  return true;
}

/**
* シーンの入力を処理する.
*/
void MainGameScene::ProcessInput()
{
  if (IsActive()) {
    GLFWEW::Window& window = GLFWEW::Window::Instance();

    const glm::aligned_vec3 dir(0, 0, -1);
    const glm::aligned_vec3 left = glm::normalize(glm::cross(glm::aligned_vec3(0, 1, 0), dir));
    const float dt = static_cast<float>(window.DeltaTime());
    glm::aligned_vec3 move(0);
    const float speed = 5.0f;
    if (window.KeyPressed(GLFW_KEY_W)) {
      move += dir * dt * speed;
    } else if (window.KeyPressed(GLFW_KEY_S)) {
      move -= dir * dt * speed;
    }
    if (window.KeyPressed(GLFW_KEY_A)) {
      move += left * dt * speed;
    } else if (window.KeyPressed(GLFW_KEY_D)) {
      move -= left * dt * speed;
    }
    if (glm::dot(move, move)) {
      meshPlayer->translation += move;
      meshPlayer->translation.y = heightMap.Height(meshPlayer->translation);
      move = glm::normalize(move);
      meshPlayer->rotation = glm::aligned_quat(glm::vec3(0, std::atan2(-move.z, move.x) + glm::radians(90.0f), 0));
      if (meshPlayer->GetAnimation() != 0) {
        meshPlayer->SetAnimation(0);
      }
    } else {
      if (meshPlayer->GetAnimation() != 1) {
        meshPlayer->SetAnimation(1);
      }
    }

    const glm::vec2 currentMousePos = GLFWEW::Window::Instance().MousePosition();
    const glm::vec2 mouseMove = currentMousePos - prevMousePos;
    prevMousePos = currentMousePos;
    glm::mat4 matRX(1);
    if (mouseMove.x) {
      matRX = glm::rotate(glm::aligned_mat4(1), -mouseMove.x / 100.0f, glm::aligned_vec3(0, 1, 0));
    }
    glm::mat4 matRY(1);
    if (mouseMove.y) {
      matRY = glm::rotate(glm::aligned_mat4(1), mouseMove.y / 100.0f, left);
    }
    //dir = matRX * matRY * glm::vec4(dir, 1);
    //dir = normalize(dir);}

      // シーン切り替え.
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttonDown & GamePad::X) {
      SceneStack::Instance().Push(std::make_shared<StatusScene>());
    } else if (gamepad.buttonDown & GamePad::START) {
      SceneStack::Instance().Replace(std::make_shared<GameOverScene>());
    }
  }
}

/**
* シーンを更新する.
*
* @param deltaTime  前回の更新からの経過時間(秒).
*
* TODO: 地面の上を移動.
* TODO: ジャンプ動作.
* TODO: 攻撃動作と攻撃判定.
* TODO: 敵の出現.
* TODO: 敵の思考.
* TODO: 木を植える.
* TODO: 村人と会話.
* TODO: ボス.
* TODO: 法力.
*/
void MainGameScene::Update(float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();

  fontRenderer.BeginUpdate();
  if (IsActive()) {
    std::wstringstream wss;
    wss << L"FPS:" << std::fixed << std::setprecision(2) << window.Fps();
    fontRenderer.AddString(glm::vec2(-600, 300), wss.str().c_str());
    fontRenderer.AddString(glm::vec2(-600, 260), L"メインゲーム画面");
  }
  fontRenderer.EndUpdate();

  meshBuffer.ResetUniformData();
  meshPlayer->Update(deltaTime);
  meshTerrain->Update(deltaTime);
  meshCircle->Update(deltaTime);
  for (auto& e : meshTrees) {
    e->Update(deltaTime * 0.25f);
  }
  for (auto& e : meshEnemies) {
    e->Update(deltaTime);
  }
  meshBuffer.UploadUniformData();
}

/**
* シーンを描画する.
*/
void MainGameScene::Render()
{
  const GLFWEW::Window& window = GLFWEW::Window::Instance();

  glm::mat4 matView;
  if (window.KeyPressed(GLFW_KEY_SPACE)) {
    const glm::aligned_vec3 front = glm::aligned_mat3(meshPlayer->rotation) * glm::aligned_vec3(0, 0, 1);
    const glm::aligned_vec3 cameraPos = meshPlayer->translation + glm::aligned_vec3(0, 2, 0);
    matView = glm::lookAt(cameraPos, meshPlayer->translation + front * 5.0f, glm::aligned_vec3(0, 1, 0));
  } else {
    const glm::aligned_vec3 cameraPos = meshPlayer->translation + glm::aligned_vec3(0, 15, 7.5f);
    matView = glm::lookAt(cameraPos, meshPlayer->translation + glm::aligned_vec3(0, 1.25f, 0), glm::aligned_vec3(0, 1, 0));
  }
  const float aspectRatio = static_cast<float>(window.Width()) / static_cast<float>(window.Height());
  const glm::mat4 matProj = glm::perspective(glm::radians(30.0f), aspectRatio, 1.0f, 1000.0f);
  const glm::mat4 matModel = glm::scale(glm::mat4(1), glm::vec3(1));

  meshBuffer.Bind();

  {
    progMesh->Use();

    progMesh->SetViewProjectionMatrix(matProj * matView * matModel);
    texTerrain->Bind(0);
    const GLint locMeshIndex = progMesh->GetUniformLocation("meshIndex");
    if (locMeshIndex > 0) {
      progMesh->SetUniformInt(locMeshIndex, 0);
    }
    meshTerrain->Draw();
    meshCircle->Draw();

    texTree->Bind(0);
    for (const auto& e : meshTrees) {
      e->Draw();
    }

    progMesh->Unuse();
  }

  {
    progSkeletalMesh->Use();

    progSkeletalMesh->SetViewProjectionMatrix(matProj * matView);
    const GLint locMeshIndex = progSkeletalMesh->GetUniformLocation("meshIndex");
    if (locMeshIndex > 0) {
      progSkeletalMesh->SetUniformInt(locMeshIndex, 0);
    }
    meshPlayer->Draw();

    texOniSmall->Bind(0);
    for (const auto& e : meshEnemies) {
      e->Draw();
    }

    progSkeletalMesh->Unuse();
  }

  texTerrain->Unbind(0);
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
