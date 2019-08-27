/**
* @file MainGameScene.cpp
*/
#include "MainGameScene.h"
#include "StatusScene.h"
#include "GameOverScene.h"
#include "../Actor/ObjectiveActor.h"
#include "../GLFWEW.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>

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
* プレイヤーと障害物の衝突を解決する.
*
* @param a  衝突したアクターその１.
* @param b  衝突したアクターその２.
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
    //const glm::vec3 normalizedVelocity = normalize(a->velocity);
    //const glm::vec3 bn = normalize(cross(a->velocity, vn));
    //const glm::vec3 projectedVelocity = normalize(cross(vn, bn)) * length(a->velocity) * 0.5f;
    //a->velocity = projectedVelocity;
    if (a->velocity.y < 0 && vn.y >= glm::cos(glm::radians(60.0f))) {
      a->velocity.y = 0;
      std::static_pointer_cast<PlayerActor>(a)->BoardToObject(b);
    } else if (a->velocity.y > 0 && vn.y <= -glm::cos(glm::radians(60.0f))) {
      a->velocity.y = 0;
    }
  } else {
    const glm::vec3 deltaVelocity = a->velocity * static_cast<float>(GLFWEW::Window::Instance().DeltaTime());
    a->position -= deltaVelocity;
    a->colWorld.s.center -= deltaVelocity;
  }
}

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

  meshBuffer.Init(sizeof(Mesh::Vertex) * 1'000'000, sizeof(GLushort) * 3'000'000, 1024);
  heightMap.Load("Res/HeightMap.tga", 50.0f, 0.5f);
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
  meshBuffer.LoadSkeletalMesh("Res/effect_hit_normal.gltf");
  meshBuffer.LoadSkeletalMesh("Res/effect_curse.gltf");
  meshBuffer.LoadSkeletalMesh("Res/watermill.gltf");

  terrain = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Terrain"), "Terrain", 100, glm::vec3(0));

  glm::vec3 startPos(100, 0, 150);
  startPos.y = heightMap.Height(startPos);
  {
    player = std::make_shared<PlayerActor>(meshBuffer.GetSkeletalMesh("Bikuni"), "Player", 20, startPos);
    player->SetHeightMap(&heightMap);
    player->SetMeshBuffer(&meshBuffer);
  }

#ifndef NDEBUG
  static const size_t treeCount = 100;
  static const size_t oniCount = 8;
  static const int oniRange = 5;
  static const size_t weedCount = 100;
  static const float vegetationRangeScale = 10.0f / 31.6f;
  const glm::vec2 vegeCenter(startPos.x, startPos.z);
#else
  static const size_t treeCount = 1000;
  static const size_t oniCount = 100;
  static const int oniRange = 20;
  static const size_t weedCount = 1000;
  static const float vegetationRangeScale = 1.0f;
  const glm::vec2 vegeCenter(glm::vec2(heightMap.Size() - 2) * 0.5f);
#endif
  rand.seed(0);
  const glm::vec2 vegeRange(glm::vec2(heightMap.Size() - 2) * 0.5f * vegetationRangeScale);
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

    {
      position = startPos + glm::vec3(20, 0, 10);
      position.y = heightMap.Height(position);
      glm::vec3 rotation(0, glm::radians(-60.0f), 0);
      SkeletalMeshActorPtr p = std::make_shared<SkeletalMeshActor>(meshBuffer.GetSkeletalMesh("Watermill"), "Watermill", 100, position, rotation);
      p->colLocal = Collision::CreateOBB({ 1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0,-1 }, { 1, 2.0f, 1.5f });
      p->GetMesh()->Play("Action");
      buildings.Add(p);
    }

    position = startPos + glm::vec3(-5, 0, 6);
    position.y = heightMap.Height(position);
    p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("JizoStatue"), "JizoStatue", 100, position, glm::vec3(0, 1.5f, 0));
    p->colLocal = Collision::CreateCapsule(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0), 0.25f);
    buildings.Add(p);

    float rotY = 0;
    position = startPos + glm::vec3(0, 0, -5);
    for (size_t i = 0; i < 10; ++i) {
      rotY += std::uniform_real_distribution<float>(-1.0f, 1.0f)(rand);
      const glm::vec3 wallDir = glm::mat3(glm::rotate(glm::mat4(1), rotY, glm::vec3(0, 1, 0))) * glm::vec3(2, 0, 0);
      position += wallDir;
      position.y = heightMap.Height(position);
      p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("StoneWall"), "StoneWall", 100, position, glm::vec3(0, rotY, 0));
      p->colLocal = Collision::CreateOBB({ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0,-1 }, { 2, 2, 0.5f });
      buildings.Add(p);
      position += wallDir;
    }

    position = startPos + glm::vec3(-20, 0, 25);
    position.y = heightMap.Height(position);
    p = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("WoodWell"), "WoodWell", 100, position, glm::vec3(0, glm::radians(30.0f), 0));
    // 軸回転テスト
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

  objectives.Reserve(4);
  for (int i = 0; i < 4; ++i) {
    glm::vec3 position(0);
    position.x = static_cast<float>(std::uniform_int_distribution<>(-oniRange, oniRange)(rand)) + startPos.x;
    position.z = static_cast<float>(std::uniform_int_distribution<>(-oniRange, oniRange)(rand)) + startPos.z;
    position.y = heightMap.Height(position);
    glm::vec3 rotation(0);
    rotation.y = std::uniform_real_distribution<float>(-glm::pi<float>() * 0.5f, glm::pi<float>() * 0.5f)(rand);
    ObjectiveActorPtr p = std::make_shared<ObjectiveActor>(meshBuffer.GetMesh("JizoStatue"), "Jizo", 100, position, rotation);
    p->scale = glm::vec3(2);
    objectives.Add(p);
  }

  effects.Reserve(100);
  for (const auto& e : objectives) {
    for (size_t i = 0; i < 1; ++i) {
      const Mesh::SkeletalMeshPtr mesh = meshBuffer.GetSkeletalMesh("CurseEffect");
      const std::vector<Mesh::Animation>& animList = mesh->GetAnimationList();
      if (!animList.empty()) {
        mesh->Play(animList[0].name);
        mesh->SetAnimationSpeed(std::uniform_real_distribution<float>(0.5f, 1.5f)(rand));
        mesh->SetPosition(std::uniform_real_distribution<float>(0.0f, 2.5f)(rand));
      }
      glm::vec3 rotation(0);
      rotation.y = std::uniform_real_distribution<float>(-glm::pi<float>() * 0.5f, glm::pi<float>() * 0.5f)(rand);
      glm::vec3 scale(std::uniform_real_distribution<float>(0.5f, 1.5f)(rand));
      SkeletalMeshActorPtr p = std::make_shared<SkeletalMeshActor>(mesh, "CurseEffect", 1, e->position, rotation, glm::vec3(0.5));
      p->scale = glm::vec3(1.25f);
      p->color = glm::vec4(1, 1, 1, 0.25f);
      effects.Add(p);
    }
  }

  SceneFader::Instance().FadeIn(1);
  return true;
}

/**
* シーンの入力を処理する.
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
    // シーン切り替え.
    const GamePad gamepad = window.GetGamePad();
    if (gamepad.buttonDown & GamePad::START) {
      SceneStack::Instance().Push(std::make_shared<StatusScene>());
//    } else if (gamepad.buttonDown & GamePad::START) {
//      SceneStack::Instance().Replace(std::make_shared<GameOverScene>());
    }
  }
}

/**
* シーンを更新する.
*
* @param deltaTime  前回の更新からの経過時間(秒).
*
* TODO: 地面の上を移動...ok
* TODO: ジャンプ動作...ok
* TODO: 攻撃動作と攻撃判定...ok
* TODO: 敵の出現.
* TODO: 敵の思考.
* TODO: 木を植える...ok
* TODO: 村人と会話.
* TODO: ボス.
* TODO: 法力.
*/
void MainGameScene::Update(float deltaTime)
{
  GLFWEW::Window& window = GLFWEW::Window::Instance();

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
    } else if (e->name == "Effect.Hit") {
      const float total = mesh->GetTotalAnimationTime();
      const float cur = mesh->GetPosition();
      const float x = std::max(0.0f, cur / total - 0.5f) * 2.0f;
      e->color.a = (1.0f - x);
    }
  }
  objectives.Update(deltaTime);

  DetectCollision(player, enemies);
  DetectCollision(player, trees);
  DetectCollision(player, buildings);
  DetectCollision(player, objectives);

  ActorPtr attackCollision = player->GetAttackCollision();
  if (attackCollision) {
    bool hit = false;
    DetectCollision(attackCollision, enemies,
      [this, &hit](const ActorPtr& a, const ActorPtr& b, const glm::vec3& p) {
        SkeletalMeshActorPtr bb = std::static_pointer_cast<SkeletalMeshActor>(b);
        bb->health -= a->health;
        float scaleFactor = 1;
        if (bb->health <= 0) {
          bb->colLocal = Collision::Shape{};
          bb->health = 1;
          bb->GetMesh()->Play("Down", false);
          scaleFactor = 1.5f;
        } else {
          bb->GetMesh()->Play("Hit", false);
        }
        std::cerr << "[INFO] enemy hp=" << bb->health << "\n";

        auto mesh = meshBuffer.GetSkeletalMesh("Effect.Hit");
        mesh->Play(mesh->GetAnimationList()[0].name, false);
        mesh->SetAnimationSpeed(std::uniform_real_distribution<float>(0.75f, 1.0f)(rand) / scaleFactor);
        glm::vec3 rot = player->rotation;
        rot.x += std::uniform_real_distribution<float>(0, glm::radians(360.0f))(rand);
        rot.y += std::uniform_real_distribution<float>(0, glm::radians(360.0f))(rand);
        rot.z += std::uniform_real_distribution<float>(0, glm::radians(360.0f))(rand);
        glm::vec3 scale(std::uniform_real_distribution<float>(1.0f, 1.5f)(rand) * scaleFactor);
        auto effect = std::make_shared<SkeletalMeshActor>(mesh, "Effect.Hit", 1, p, rot, scale * b->scale);
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
  objectives.UpdateDrawData(deltaTime);

  {
    fontRenderer.BeginUpdate();
    fontRenderer.Scale(glm::vec2(1));
    const float hh = static_cast<float>(window.Height()) * 0.5f;
    const float lh = fontRenderer.LineHeight();
    if (IsActive()) {
      fontRenderer.AddString(glm::vec2(380, hh - lh), L"メインゲーム画面");
      std::wstringstream wss;
      wss << L"FPS:" << std::fixed << std::setprecision(2) << window.Fps();
      fontRenderer.AddString(glm::vec2(500, hh - lh * 2), wss.str().c_str());
    }
    {
      std::wstringstream wss;
      wss << L"体力：";
      for (int i = 0; i < player->health; i += 4) {
        wss << L"●";
      }
      fontRenderer.AddString(glm::vec2(-600, hh - lh * 2), wss.str().c_str());
    }

    if (isMissionAccomplished) {
      fontRenderer.Scale(glm::vec2(2));
      fontRenderer.AddString(glm::vec2(-400, 0), L"全ての地蔵の呪いを解いた！");
    } else {
      int achievementCount = 0;
      for (const auto& e : objectives) {
        ObjectiveActorPtr p = std::static_pointer_cast<ObjectiveActor>(e);
        if (p->IsAchieved()) {
          ++achievementCount;
        }
      }
      if (achievementCount >= 4) {
        isMissionAccomplished = true;
        timer = 3;
      }
    }
    fontRenderer.EndUpdate();
  }

  if (isMissionAccomplished) {
    if (timer > 0) {
      timer -= deltaTime;
      if (timer <= 0) {
        SceneFader::Instance().FadeOut(1);
      }
    } else if (!SceneFader::Instance().IsFading()) {
      SceneStack::Instance().Replace(std::make_shared<GameOverScene>());
    }
  }
}

/**
* シーンを描画する.
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
    const glm::vec3 cameraPos = player->position + glm::vec3(0, 50, 50) * 0.25f;
    matView = glm::lookAt(cameraPos, player->position + glm::vec3(0, 1.25f, 0), glm::vec3(0, 1, 0));
  }
  const float aspectRatio = static_cast<float>(window.Width()) / static_cast<float>(window.Height());
  const glm::mat4 matProj = glm::perspective(glm::radians(30.0f), aspectRatio, 1.0f, 1000.0f);

  meshBuffer.SetViewProjectionMatrix(matProj * matView);
  {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    terrain->Draw();
    buildings.Draw();

    glDisable(GL_CULL_FACE);

    trees.Draw();
    vegetations.Draw();

    glEnable(GL_CULL_FACE);

    player->Draw();
    enemies.Draw();
    objectives.Draw();

    glDisable(GL_CULL_FACE);
    effects.Draw();
  }

  const glm::vec2 screenSize(window.Width(), window.Height());
  fontRenderer.Draw(screenSize);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
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
  Scene::Play();
}
