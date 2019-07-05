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
  ���b�Z�[�W���[�h�̊J�n begin ���x��
  msg �e�L�X�g
  select �I����
  set �ϐ��ԍ� ���lor�ϐ��ԍ�
  add �ϐ��ԍ� ���lor�ϐ��ԍ�
  sub �ϐ��ԍ� ���lor�ϐ��ԍ�
  if �ϐ��ԍ�(��0�łȂ����) �W�����v�惉�x��
  ���b�Z�[�W���[�h�̏I�� end

  ����̍u�`�\��:
  - ���b�V���\��
  - �n�C�g�}�b�v
  - glTF���b�V���\��(�{�[�i�X�g���b�N:�@�A�j���[�V����)
  - ���Ƌ��̏Փ˔���
  - ��`�Ƌ��̏Փ˔���
  - �C�x���g����(��b�A�t���O����)
*/

/**
* �R���X�g���N�^.
*/
MainGameScene::MainGameScene() : Scene("MainGameScene")
{
}

/**
* �V�[��������������.
*
* @retval true  ����������.
* @retval false ���������s. �Q�[���i�s�s�ɂ��A�v���O�������I�����邱��.
*/
bool MainGameScene::Initialize()
{
  fontRenderer.Init(1000);
  fontRenderer.LoadFromFile("Res/font.fnt");

  meshBuffer.Init(sizeof(Mesh::Vertex) * 1'000'000, sizeof(GLushort) * 3'000'000, 1024);
  heightMap.Load("Res/HeightMap.tga", 100.0f, 0.5f);
  heightMap.CreateMesh(meshBuffer, "Terrain", "Res/ColorMap.tga");

  meshBuffer.LoadMesh("Res/oni_small.gltf");
  meshBuffer.LoadMesh("Res/bikuni_ver2.gltf");
  meshBuffer.LoadMesh("Res/weed_collection.gltf");
  meshBuffer.LoadMesh("Res/red_pine_tree.gltf");
  meshBuffer.LoadMesh("Res/farmers_house.gltf");
  meshBuffer.LoadMesh("Res/jizo_statue.gltf");
  meshBuffer.LoadMesh("Res/temple.gltf");

  terrain = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Terrain"), "Terrain", 100, glm::vec3(0));

  // SkeletalMesh�N���X�̃e�X�g�R�[�h.
  skeletalMeshTest = meshBuffer.GetSkeletalMesh("Bikuni");
  skeletalMeshTest->Play("Idle");

  glm::vec3 startPos(100, 0, 150);
  startPos.y = heightMap.Height(startPos);
  player = std::make_shared<SkeletalMeshActor>(meshBuffer.GetSkeletalMesh("Bikuni"), "Player", 20, startPos);
  player->GetMesh()->Play("Idle");

#ifndef NDEBUG
  static const size_t treeCount = 100;
  static const size_t oniCount = 8;
  static const int oniRange = 5;
  static const size_t weedCount = 100;
#else
  static const size_t treeCount = 1000;
  static const size_t oniCount = 100;
  static const int oniRange = 20;
  static const size_t weedCount = 1000;
#endif
  std::mt19937 rand;
  rand.seed(0);
  {
    trees.Reserve(treeCount);
    const Mesh::MeshPtr mesh = meshBuffer.GetMesh("RedPineTree");
    for (size_t i = 0; i < treeCount; ++i) {
      glm::vec3 position(0);
      position.x = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().x - 2)(rand));
      position.z = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().y - 2)(rand));
      position.y = heightMap.Height(position);
      glm::vec3 rotation(0);
      rotation.y = std::uniform_real_distribution<float>(0, glm::two_pi<float>())(rand);
      glm::vec3 scale = glm::vec3(std::normal_distribution<float>(0.7f, 0.2f)(rand));
      scale = glm::clamp(scale, 0.6f, 1.4f);
      trees.Add(std::make_shared<StaticMeshActor>(mesh, "Tree", 100, position, rotation, scale));
    }
  }

  {
    vegetations.Reserve(weedCount);
    Mesh::MeshPtr mesh[3] = {
      meshBuffer.GetMesh("Weed.Susuki"),
      meshBuffer.GetMesh("Weed.Kazekusa"),
      meshBuffer.GetMesh("Weed.Chigaya")
    };
    for (size_t i = 0; i < weedCount; ++i) {
      glm::vec3 position(0);
      position.x = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().x - 2)(rand));
      position.z = static_cast<float>(std::uniform_int_distribution<>(1, heightMap.Size().y - 2)(rand));
      position.y = heightMap.Height(position);
      glm::vec3 rotation(0);
      rotation.y = std::uniform_real_distribution<float>(0, glm::two_pi<float>())(rand);
      glm::vec3 scale = glm::vec3(std::normal_distribution<float>(0.8f, 0.2f)(rand));
      scale = glm::clamp(scale, 0.8f, 1.2f);
      vegetations.Add(std::make_shared<StaticMeshActor>(mesh[i % 3], mesh[i % 3]->name, 100, position, rotation, scale));
    }
  }

  {
    buildings.Reserve(10);
    glm::vec3 position = startPos + glm::vec3(20, 5, 3);
    position.y = heightMap.Height(position);
    buildings.Add(std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("FarmersHouse"), "FarmersHouse", 100, position));

    position = startPos + glm::vec3(12, 0, 20);
    position.y = heightMap.Height(position);
    buildings.Add(std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Temple"), "Temple", 100, position));

    position = startPos + glm::vec3(-5, 0, 6);
    position.y = heightMap.Height(position);
    buildings.Add(std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("JizoStatue"), "JizoStatue", 100, position, glm::vec3(0, 1.5f, 0)));
  }

  {
    enemies.Reserve(oniCount);
    for (size_t i = 0; i < oniCount; ++i) {
      glm::vec3 position(0);
      position.x = static_cast<float>(std::uniform_int_distribution<>(-oniRange, oniRange)(rand)) + startPos.x;
      position.z = static_cast<float>(std::uniform_int_distribution<>(-oniRange, oniRange)(rand)) + startPos.z;
      position.y = heightMap.Height(position);
      glm::vec3 rotation(0);
      rotation.y = std::uniform_real_distribution<float>(0, glm::two_pi<float>())(rand);
      glm::vec3 scale = glm::vec3(std::normal_distribution<float>(0.0f, 1.0f)(rand) + 0.5f);
      scale = glm::clamp(scale, 0.75f, 1.25f);
      const Mesh::SkeletalMeshPtr mesh = meshBuffer.GetSkeletalMesh("oni_small");
      const std::vector<Mesh::Animation>& animList = mesh->GetAnimationList();
      mesh->Play(animList[i % animList.size()].name);
      enemies.Add(std::make_shared<SkeletalMeshActor>(mesh, "Kooni", 13, position, rotation, scale));
    }
  }

  return true;
}

/**
* �V�[���̓��͂���������.
*/
void MainGameScene::ProcessInput()
{
  if (IsActive()) {
    GLFWEW::Window& window = GLFWEW::Window::Instance();

    if (actionWaitTimer <= 0) {
      if (window.GetGamePad().buttonDown & GamePad::A) {
        player->GetMesh()->Play("Attack.Light", false);
        actionWaitTimer = player->GetMesh()->GetTotalAnimationTime();
      } else if (window.GetGamePad().buttonDown & GamePad::B) {
        player->GetMesh()->Play("Attack.Heavy", false);
        actionWaitTimer = player->GetMesh()->GetTotalAnimationTime();
      } else {
        const glm::vec3 dir(0, 0, -1);
        const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
        glm::vec3 move(0);
        const float speed = 5.0f;
        if (window.KeyPressed(GLFW_KEY_W)) {
          move += dir * speed;
        } else if (window.KeyPressed(GLFW_KEY_S)) {
          move -= dir * speed;
        }
        if (window.KeyPressed(GLFW_KEY_A)) {
          move += left * speed;
        } else if (window.KeyPressed(GLFW_KEY_D)) {
          move -= left * speed;
        }
        if (glm::dot(move, move)) {
          player->velocity = move;
          move = glm::normalize(move);
          player->rotation.y = std::atan2(-move.z, move.x) + glm::radians(90.0f);
        }
      }
    }

    // �V�[���؂�ւ�.
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttonDown & GamePad::X) {
      SceneStack::Instance().Push(std::make_shared<StatusScene>());
    } else if (gamepad.buttonDown & GamePad::START) {
      SceneStack::Instance().Replace(std::make_shared<GameOverScene>());
    }
  }
}

/**
* �V�[�����X�V����.
*
* @param deltaTime  �O��̍X�V����̌o�ߎ���(�b).
*
* TODO: �n�ʂ̏���ړ�...ok
* TODO: �W�����v����.
* TODO: �U������ƍU������.
* TODO: �G�̏o��.
* TODO: �G�̎v�l.
* TODO: �؂�A����...ok
* TODO: ���l�Ɖ�b.
* TODO: �{�X.
* TODO: �@��.
*/
void MainGameScene::Update(float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();

  fontRenderer.BeginUpdate();
  if (IsActive()) {
    std::wstringstream wss;
    wss << L"FPS:" << std::fixed << std::setprecision(2) << window.Fps();
    fontRenderer.AddString(glm::vec2(-600, 300), wss.str().c_str());
    fontRenderer.AddString(glm::vec2(-600, 260), L"���C���Q�[�����");
  }
  fontRenderer.EndUpdate();

  if (actionWaitTimer > 0) {
    actionWaitTimer -= deltaTime;
  }

  player->Update(deltaTime);
  player->position.y = heightMap.Height(player->position);
  if (player->GetMesh()->GetAnimation() == "Idle") {
    if (glm::length(player->velocity) >= 1.0f * deltaTime) {
      player->GetMesh()->Play("Run");
    }
  } else if (player->GetMesh()->GetAnimation() == "Run") {
    if (glm::dot(player->velocity, player->velocity) == 0.0f) {
      player->GetMesh()->Play("Idle");
    } else {
      const glm::vec3 oldVelocity = player->velocity;
      player->velocity -= glm::normalize(player->velocity) * 60.0f * deltaTime;
      if (glm::dot(player->velocity, oldVelocity) <= 0.0f) {
        player->velocity = glm::vec3(0);
      }
    }
  }
  player->UpdateDrawData(deltaTime);

  terrain->Update(deltaTime);
  terrain->UpdateDrawData(deltaTime);
  trees.Update(deltaTime);
  trees.UpdateDrawData(deltaTime);
  vegetations.Update(deltaTime);
  vegetations.UpdateDrawData(deltaTime);
  buildings.Update(deltaTime);
  buildings.UpdateDrawData(deltaTime);
  enemies.Update(deltaTime);
  enemies.UpdateDrawData(deltaTime);

  // SkeletalMesh�N���X�̃e�X�g�R�[�h.
  {
    glm::vec3 pos(120, 0, 160);
    pos.y = heightMap.Height(pos);
    const glm::mat4 matModel = translate(glm::mat4(1), pos);
    skeletalMeshTest->Update(deltaTime, matModel, glm::vec4(1.5f, 0.5f, 0.5f, 1.0f));
  }
}

/**
* �V�[����`�悷��.
*/
void MainGameScene::Render()
{
  const GLFWEW::Window& window = GLFWEW::Window::Instance();

  glm::mat4 matView;
  if (window.KeyPressed(GLFW_KEY_SPACE)) {
    const glm::aligned_vec3 front = glm::rotate(glm::mat4(1), player->rotation.y, glm::vec3(0, 1, 0)) * glm::aligned_vec4(0, 0, 1, 1);
    const glm::aligned_vec3 cameraPos = player->position + glm::vec3(0, 1.3f, 0);
    matView = glm::lookAt(cameraPos, cameraPos + front * 5.0f, glm::aligned_vec3(0, 1, 0));
  } else {
    const glm::vec3 cameraPos = player->position + glm::vec3(0, 50, 50);// *0.25f;
    matView = glm::lookAt(cameraPos, player->position + glm::vec3(0, 1.25f, 0), glm::vec3(0, 1, 0));
  }
  const float aspectRatio = static_cast<float>(window.Width()) / static_cast<float>(window.Height());
  const glm::mat4 matProj = glm::perspective(glm::radians(30.0f), aspectRatio, 1.0f, 1000.0f);

  meshBuffer.Bind();
  meshBuffer.SetViewProjectionMatrix(matProj * matView);

  {
    glEnable(GL_CULL_FACE);

    terrain->Draw();
    buildings.Draw();

    glDisable(GL_CULL_FACE);

    trees.Draw();
    vegetations.Draw();
  }

  {
    glEnable(GL_CULL_FACE);

    player->Draw();
    enemies.Draw();
  }

  // SkeletalMesh�N���X�̃e�X�g�R�[�h.
  skeletalMeshTest->Draw();

  meshBuffer.Unbind();

  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);
}

/**
* �V�[����j������.
*/
void MainGameScene::Finalize()
{
  GLFWEW::Window::Instance().EnableMouseCursor();
}

/**
* �V�[����������Ԃɂ���.
*/
void MainGameScene::Play()
{
  GLFWEW::Window::Instance().DisableMouseCursor();
  Scene::Play();
}
