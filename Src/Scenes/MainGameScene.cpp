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
#include <iostream>

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
* �v���C���[�Ə�Q���̏Փ˂���������.
*
* @param a  �Փ˂����A�N�^�[���̂P.
* @param b  �Փ˂����A�N�^�[���̂Q.
*/
void PlayerObstacleCollisionHandler(const ActorPtr& a, const ActorPtr& b, const glm::vec3& p)
{
  const glm::vec3 v = a->colWorld.s.center - p;
  if (dot(v, v) > FLT_EPSILON) {
    const glm::vec3 vn = normalize(v);
    float r = a->colWorld.s.r;
    switch (b->colWorld.type) {
    case Collision::Shape::Type::sphere: r += b->colWorld.s.r; break;
    case Collision::Shape::Type::capsule: r += b->colWorld.c.r; break;
    case Collision::Shape::Type::obb: break;
    }
    const float distance = r - glm::length(v) + 0.01f;
    a->position += vn * distance;
    a->colWorld.s.center += vn * distance;
    if (a->velocity.y < 0 && v.y > FLT_EPSILON) {
      a->velocity.y = 0;
    } else if (a->velocity.y > 0 && v.y < FLT_EPSILON) {
      a->velocity.y = 0;
    }
  } else {
    const glm::vec3 deltaVelocity = a->velocity * static_cast<float>(GLFWEW::Window::Instance().DeltaTime());
    a->position -= deltaVelocity;
    a->colWorld.s.center -= deltaVelocity;
  }
}

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

  meshBuffer.CreateSphere("Sphere", 8, 8);
  meshBuffer.LoadSkeletalMesh("Res/oni_small.gltf");
  meshBuffer.LoadSkeletalMesh("Res/oni_medium.gltf");
  meshBuffer.LoadSkeletalMesh("Res/bikuni.gltf");
  meshBuffer.LoadMesh("Res/weed_collection.gltf");
  meshBuffer.LoadMesh("Res/red_pine_tree.gltf");
  meshBuffer.LoadMesh("Res/farmers_house.gltf");
  meshBuffer.LoadMesh("Res/jizo_statue.gltf");
  meshBuffer.LoadMesh("Res/temple.gltf");
  meshBuffer.LoadMesh("Res/wood_well.gltf");
  meshBuffer.LoadMesh("Res/wall_stone.gltf");
  meshBuffer.LoadSkeletalMesh("Res/effect_hit.gltf");

  terrain = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Terrain"), "Terrain", 100, glm::vec3(0));

  glm::vec3 startPos(100, 0, 150);
  startPos.y = heightMap.Height(startPos);
  {
    player = std::make_shared<PlayerActor>(meshBuffer.GetSkeletalMesh("Bikuni"), "Player", 20, startPos);
    player->SetHeightMap(&heightMap);
    player->SetMeshBuffer(&meshBuffer);
    player->GetMesh()->Play("Idle");
    player->colLocal = Collision::CreateSphere(glm::vec3(0, 0.7f, 0), 0.5f);
  }

#ifndef NDEBUG
  static const size_t treeCount = 100;
  static const size_t oniCount = 8;
  static const int oniRange = 5;
  static const size_t weedCount = 100;
  static const float vegetationRangeScale = 10.0f / 31.6f;
#else
  static const size_t treeCount = 1000;
  static const size_t oniCount = 100;
  static const int oniRange = 20;
  static const size_t weedCount = 1000;
  static const int vegetationRangeScale = 1.0f;
#endif
  std::mt19937 rand;
  rand.seed(0);
  const glm::vec2 vegeRange(glm::vec2(heightMap.Size() - 2) * 0.5f * vegetationRangeScale);
  const glm::vec2 vegeCenter(startPos.x, startPos.z);
  const glm::vec2 vegeRangeMin = vegeCenter - vegeRange;
  const glm::vec2 vegeRangeMax = vegeCenter + vegeRange;
  {

    trees.Reserve(treeCount);
    const Mesh::MeshPtr mesh = meshBuffer.GetMesh("RedPineTree");
    for (size_t i = 0; i < treeCount; ++i) {
      glm::vec3 position(0);
      position.x = std::uniform_real_distribution<float>(vegeRangeMin.x, vegeRangeMax.x)(rand);
      position.z = std::uniform_real_distribution<float>(vegeRangeMin.y, vegeRangeMax.y)(rand);
      position.y = heightMap.Height(position);
      glm::vec3 rotation(0);
      rotation.y = std::uniform_real_distribution<float>(0, glm::two_pi<float>())(rand);
      glm::vec3 scale = glm::vec3(std::normal_distribution<float>(0.7f, 0.2f)(rand));
      scale = glm::clamp(scale, 0.6f, 1.4f);
      StaticMeshActorPtr p = std::make_shared<StaticMeshActor>(mesh, "Tree", 100, position, rotation, scale);
      p->colLocal = Collision::CreateCapsule( glm::vec3(0), glm::vec3(0, 4, 0), 0.5f);
      trees.Add(p);
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
      position.x = std::uniform_real_distribution<float>(vegeRangeMin.x, vegeRangeMax.x)(rand);
      position.z = std::uniform_real_distribution<float>(vegeRangeMin.y, vegeRangeMax.y)(rand);
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
    StaticMeshActorPtr p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("FarmersHouse"), "FarmersHouse", 100, position);
    p->colLocal = Collision::CreateOBB({ 0, 1, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0,0,-1 }, { 2.5f, 3, 2.0f });
    buildings.Add(p);

    position = startPos + glm::vec3(12, 0, 20);
    position.y = heightMap.Height(position);
    buildings.Add(std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Temple"), "Temple", 100, position));

    position = startPos + glm::vec3(-5, 0, 6);
    position.y = heightMap.Height(position);
    p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("JizoStatue"), "JizoStatue", 100, position, glm::vec3(0, 1.5f, 0));
    p->colLocal = Collision::CreateCapsule(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0), 0.25f);
    buildings.Add(p);

    position = startPos + glm::vec3(20, 0, 10);
    position.y = heightMap.Height(position);
    p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("StoneWall"), "StoneWall", 100, position, glm::vec3(0, -0.5f, 0));
    p->colLocal = Collision::CreateOBB({ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0,-1 }, { 2, 2, 0.5f });
    buildings.Add(p);

    position = startPos + glm::vec3(-20, 0, 25);
    position.y = heightMap.Height(position);
    p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("WoodWell"), "WoodWell", 100, position, glm::vec3(0, glm::radians(30.0f), 0));
    // ����]�e�X�g
    const glm::mat4 matR = glm::rotate(glm::mat4(1), glm::radians(0.0f), glm::vec3(0, 1, 0));
    p->colLocal = Collision::CreateOBB({ 0, 0, 0 }, matR * glm::vec4(1, 0, 0, 1), matR * glm::vec4(0, 1, 0, 1), matR * glm::vec4(0, 0,-1, 1), {1, 2, 1});
    buildings.Add(p);
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
      Mesh::SkeletalMeshPtr mesh;
      if (i % 5) {
        mesh = meshBuffer.GetSkeletalMesh("oni_small");
      } else {
        mesh = meshBuffer.GetSkeletalMesh("OniMedium");
      }
      mesh->Play("Wait");
      SkeletalMeshActorPtr p = std::make_shared<SkeletalMeshActor>(mesh, "Kooni", 13, position, rotation, scale);
      p->colLocal = Collision::CreateCapsule({ 0, 0.5f, 0 }, {0, 1, 0}, 0.5f);
      enemies.Add(p);
    }
  }

  effects.Reserve(100);
  {
    glm::vec3 position(startPos + glm::vec3(3, 0, -6));
    position.y = heightMap.Height(position) + 1;
    const Mesh::SkeletalMeshPtr mesh = meshBuffer.GetSkeletalMesh("HitEffect");
    const std::vector<Mesh::Animation>& animList = mesh->GetAnimationList();
    if (!animList.empty()) {
      mesh->Play(animList[0].name);
    }
    SkeletalMeshActorPtr p = std::make_shared<SkeletalMeshActor>(mesh, "HitEffect", 1, position, glm::vec3(0), glm::vec3(0.5));
    effects.Add(p);
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
#if 1
    player->ProcessInput();
#else
    if (actionWaitTimer <= 0) {
      const GamePad gamepad = window.GetGamePad();
      if (gamepad.buttonDown & GamePad::A) {
        player->GetMesh()->Play("Attack.Light", false);
        actionWaitTimer = player->GetMesh()->GetTotalAnimationTime();
      } else if (gamepad.buttonDown & GamePad::X) {
        player->GetMesh()->Play("Attack.Heavy", false);
        actionWaitTimer = player->GetMesh()->GetTotalAnimationTime();
      } else {
        const glm::vec3 dir(0, 0, -1);
        const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
        glm::vec3 move(0);
        const float speed = 5.0f;
        if (gamepad.buttons & GamePad::DPAD_UP) {
          move += dir * speed;
        } else if (gamepad.buttons & GamePad::DPAD_DOWN) {
          move -= dir * speed;
        }
        if (gamepad.buttons & GamePad::DPAD_LEFT) {
          move += left * speed;
        } else if (gamepad.buttons & GamePad::DPAD_RIGHT) {
          move -= left * speed;
        }
        if (glm::dot(move, move)) {
          player->velocity.x = move.x;
          player->velocity.z = move.z;
          move = glm::normalize(move);
          player->rotation.y = std::atan2(-move.z, move.x) + glm::radians(90.0f);
        }
        if (gamepad.buttonDown & GamePad::B) {
          player->velocity.y += 5.0f;
        }
      }
    }
#endif
    // �V�[���؂�ւ�.
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttonDown & GamePad::START) {
      SceneStack::Instance().Push(std::make_shared<StatusScene>());
//    } else if (gamepad.buttonDown & GamePad::START) {
//      SceneStack::Instance().Replace(std::make_shared<GameOverScene>());
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
  terrain->Update(deltaTime);
  trees.Update(deltaTime);
  vegetations.Update(deltaTime);
  buildings.Update(deltaTime);
  enemies.Update(deltaTime);
  for (auto& e : enemies) {
    SkeletalMeshActorPtr enemy = std::static_pointer_cast<SkeletalMeshActor>(e);
    Mesh::SkeletalMeshPtr mesh = enemy->GetMesh();
    if (mesh->IsFinished()) {
      if (mesh->GetAnimation() == "Down") {
        enemy->health = 0;
      } else {
        mesh->Play("Wait");
      }
    }
  }
  effects.Update(deltaTime);
  for (auto& e : effects) {
    Mesh::SkeletalMeshPtr mesh = std::static_pointer_cast<SkeletalMeshActor>(e)->GetMesh();
    if (mesh->IsFinished()) {
      e->health = 0;
    }
  }

  DetectCollision(player, enemies, PlayerObstacleCollisionHandler);
  DetectCollision(player, trees, PlayerObstacleCollisionHandler);
  DetectCollision(player, buildings, PlayerObstacleCollisionHandler);

  ActorPtr attackCollision = player->GetAttackCollision();
  if (attackCollision) {
    bool hit = false;
    DetectCollision(attackCollision, enemies,
      [this, &hit](const ActorPtr& a, const ActorPtr& b, const glm::vec3& p) {
        SkeletalMeshActorPtr bb = std::static_pointer_cast<SkeletalMeshActor>(b);
        bb->health -= a->health;
        if (bb->health <= 0) {
          bb->colLocal = Collision::Shape{};
          bb->health = 1;
          bb->GetMesh()->Play("Down", false);
        } else {
          bb->GetMesh()->Play("Hit", false);
        }
        std::cerr << "[INFO] enemy hp=" << bb->health << "\n";

        auto mesh = meshBuffer.GetSkeletalMesh("HitEffect");
        mesh->Play(mesh->GetAnimationList()[0].name, false);
        glm::vec3 rot = player->rotation;
        rot.y += glm::radians(180.0f);
        auto effect = std::make_shared<SkeletalMeshActor>(mesh, "HitEffect", 1, p, rot, glm::vec3(0.75f));
        effects.Add(effect);
        hit = true;
      }
    );
    if (hit) {
      attackCollision->health = 0;
    }
  }
  enemies.RemoveDead();
  effects.RemoveDead();

#if 1
#else
  {
    const glm::vec3 velocityXZ(player->velocity.x, 0, player->velocity.z);
    if (glm::dot(velocityXZ, velocityXZ) <= FLT_EPSILON) {
      player->velocity *= glm::vec3(0, 1, 0);
    } else {
      player->velocity -= glm::normalize(velocityXZ) * 60.0f * deltaTime;
      if (glm::dot(glm::vec3(player->velocity.x, 0, player->velocity.z), velocityXZ) <= 0.0f) {
        player->velocity *= glm::vec3(0, 1, 0);
      }
    }
    const float groundY = heightMap.Height(player->position);
    if (player->position.y < groundY) {
      player->position.y = groundY;
      player->velocity.y = 0;
    } else if (player->position.y > groundY) {
      player->velocity.y -= 9.8f * deltaTime;
    }

    if (actionWaitTimer <= 0) {
      if (player->GetMesh()->GetAnimation() == "Idle") {
        if (glm::length(velocityXZ) >= 1.0f * deltaTime) {
          player->GetMesh()->Play("Run");
        }
      } else if (player->GetMesh()->GetAnimation() == "Run") {
        if (glm::length(velocityXZ) < 1.0f * deltaTime) {
          player->GetMesh()->Play("Idle");
        }
      } else {
        player->GetMesh()->Play("Idle");
      }
    }
  }
#endif

  player->UpdateDrawData(deltaTime);
  terrain->UpdateDrawData(deltaTime);
  trees.UpdateDrawData(deltaTime);
  vegetations.UpdateDrawData(deltaTime);
  buildings.UpdateDrawData(deltaTime);
  enemies.UpdateDrawData(deltaTime);
  effects.UpdateDrawData(deltaTime);
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

  meshBuffer.SetViewProjectionMatrix(matProj * matView);
  {
    glEnable(GL_CULL_FACE);

    terrain->Draw();
    buildings.Draw();

    glDisable(GL_CULL_FACE);

    trees.Draw();
    vegetations.Draw();

    glEnable(GL_CULL_FACE);

    player->Draw();
    enemies.Draw();
    effects.Draw();
  }

  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
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
