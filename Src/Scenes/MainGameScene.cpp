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
* プレイヤーが操作するアクター.
*/
class PlayerActor : public SkeletalMeshActor
{
public:
  PlayerActor(const Mesh::SkeletalMeshPtr& m, const std::string& name, int hp, const glm::vec3& pos,
    const glm::vec3& rot = glm::vec3(0), const glm::vec3& scale = glm::vec3(1));
  virtual ~PlayerActor() = default;

  virtual void Update(float) override;
  void ProcessInput();
  void SetHeightMap(const Terrain::HeightMap* p) { heightMap = p; }

private:
  bool CheckRun(const GamePad& gamepad);
  bool CheckAttack(const GamePad& gamepad);
  bool CheckGuard(const GamePad& gamepad);
  bool CheckJump(const GamePad& gamepad);
  bool CheckFall(float, float);

  enum class State {
    idle,
    run,
    jump,
    lightAttack,
    heavyAttack,
    jumpAttack,
    guard,
    damage,
    dead,
  };
  void SetState(State s) {
    static const char* const names[] = {
      "idle", "run", "jump", "lightAttack", "heavyAttack", "jumpAttack", "guard", "damage", "dead"
    };
    if (state != s) {
      std::cout << "[INFO]" << __func__ << ":" << names[static_cast<int>(state)] << "->" << names[static_cast<int>(s)] << "\n";
      state = s;
    }
  }

  State state = State::idle;
  float stateTimer = 0;
  bool isGrounded = false;
  Collision::Sphere  colWorldAttack;
  const Terrain::HeightMap* heightMap = nullptr;
};

/**
*
*/
PlayerActor::PlayerActor(const Mesh::SkeletalMeshPtr& m, const std::string& name, int hp, const glm::vec3& pos,
  const glm::vec3& rot, const glm::vec3& scale)
  : SkeletalMeshActor(m, name, hp, pos, rot, scale)
{
  std::cout << "[INFO]" << __func__ << ": idle\n";
}

glm::vec3 GetMove(const GamePad& gamepad)
{
  const glm::vec3 dir(0, 0, -1);
  const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0, 1, 0), dir));
  glm::vec3 move(0);
  if (gamepad.buttons & GamePad::DPAD_UP) {
    move += dir;
  } else if (gamepad.buttons & GamePad::DPAD_DOWN) {
    move -= dir;
  }
  if (gamepad.buttons & GamePad::DPAD_LEFT) {
    move += left;
  } else if (gamepad.buttons & GamePad::DPAD_RIGHT) {
    move -= left;
  }
  return move;
}

/**
*
*/
bool PlayerActor::CheckRun(const GamePad& gamepad)
{
  glm::vec3 move = GetMove(gamepad);
  if (glm::dot(move, move)) {
    move = glm::normalize(move);
    rotation.y = std::atan2(-move.z, move.x) + glm::radians(90.0f);

    const glm::vec3 right = glm::normalize(glm::cross(move, glm::vec3(0, 1, 0)));
    const float frontY = heightMap->Height(position + move * 0.1f) - position.y - 0.01f;
    const float rad = glm::clamp(std::atan2(frontY, 0.1f), glm::radians(-45.0f), glm::radians(60.0f));
    move = glm::rotate(glm::mat4(1), rad, right) * glm::vec4(move, 1.0f);

    const float speed = 5.0f;
    velocity = move * speed;

    if (GetMesh()->GetAnimation() != "Run") {
      GetMesh()->Play("Run");
    }
    SetState(State::run);
    return true;
  }
  return false;
}

/**
*
*/
bool PlayerActor::CheckAttack(const GamePad& gamepad)
{
  if (gamepad.buttonDown & GamePad::A) {
    GetMesh()->Play("Attack.Light", false);
    stateTimer = GetMesh()->GetTotalAnimationTime();
    SetState(State::lightAttack);
    return true;
  }
  return false;
}

/**
*
*/
bool PlayerActor::CheckGuard(const GamePad& gamepad)
{
  if (gamepad.buttons & GamePad::X) {
    GetMesh()->Play("Guard", false);
    stateTimer = GetMesh()->GetTotalAnimationTime();
    SetState(State::guard);
    return true;
  }
  return false;
}

/**
*
*/
bool PlayerActor::CheckJump(const GamePad& gamepad)
{
  if (isGrounded && (gamepad.buttonDown & GamePad::B)) {
    GetMesh()->Play("Jump");
    static const float jumpVelocity = 5.0f;
    velocity.y = jumpVelocity;
    isGrounded = false;
    SetState(State::jump);
    return true;
  }
  return false;
}

/**
*
*/
bool PlayerActor::CheckFall(float deltaTime, float groundHeight)
{
  if ((position.y - groundHeight >= 0.1f) && (velocity.y < -0.1f)) {
    if (GetMesh()->GetAnimation() != "Fall") {
      GetMesh()->Play("Fall");
      isGrounded = false;
      std::cout << "Fall";
      SetState(State::jump);
      return true;
    }
  }
  return false;
}

/**
*
*/
void PlayerActor::ProcessInput()
{
  const Mesh::SkeletalMeshPtr& mesh = GetMesh();
  const GamePad gamepad = GLFWEW::Window::Instance().GetGamePad();
  switch (state) {
  case State::idle:
    // A -> attack
    if (CheckAttack(gamepad)) {
      // X -> guard
    } else if (CheckGuard(gamepad)) {
    } else {
      // DPAD -> run
      CheckRun(gamepad);
      // B -> jump
      CheckJump(gamepad);
    }
    break;
  case State::run:
    // A -> attack
    if (CheckAttack(gamepad)) {
      // X -> guard
    } else if (CheckGuard(gamepad)) {
    } else {
      // DPAD -> run
      CheckRun(gamepad);
      // B -> jump
      CheckJump(gamepad);
    }
    break;
  case State::jump:
    // A -> jumpAttack
    if (gamepad.buttonDown & GamePad::A) {
      mesh->Play("Attack.Jump", false);
      stateTimer = mesh->GetTotalAnimationTime();
      SetState(State::jumpAttack);
    }
    break;
  case State::lightAttack:
    // EOA(End Of Animation) -> idle
    break;
  case State::heavyAttack:
    // EOA -> idle
    break;
  case State::jumpAttack:
    // EOA -> jump
    break;
  case State::guard: {
    glm::vec3 move = GetMove(gamepad);
    if (dot(move, move)) {
      move = glm::normalize(move);
      rotation.y = std::atan2(-move.z, move.x) + glm::radians(90.0f);
    }
    // Release X -> idle;
    if (!(gamepad.buttons & GamePad::X)) {
      mesh->Play("Idle");
      SetState(State::idle);
    }
    break;
  }
  case State::damage:
    // EOA -> idle
    break;
  case State::dead:
    // Timer -> game over
    break;
  }
}

/**
*
*/
void PlayerActor::Update(float deltaTime)
{
  // 座標の更新.
  SkeletalMeshActor::Update(deltaTime);

  // 接地判定.
  static const float gravity = 9.8f;
  const float groundHeight = heightMap->Height(position);
  if (position.y <= groundHeight) {
    position.y = groundHeight;
    if (!isGrounded) {
      velocity.y = 0;
      isGrounded = true;
    }
  } else if (position.y > groundHeight) {
    velocity.y -= gravity * deltaTime;
    isGrounded = false;
  }

  // 速度の減衰.
  if (isGrounded) {
    const glm::vec3 velocityXZ(velocity.x, 0, velocity.z);
    if (glm::dot(velocity, velocity) <= FLT_EPSILON) {
      velocity = glm::vec3(0);
    } else {
      static const float dampingFactor = 30.0f;
      glm::vec3 old = velocity;
      velocity -= glm::normalize(velocity) * dampingFactor * deltaTime;
      if (glm::dot(velocity, old) <= 0.0f) {
        velocity = glm::vec3(0);
      }
    }
  }

  // 状態の更新.
  switch (state) {
  case State::run:
    if (CheckFall(deltaTime, groundHeight)) {
    } else if (glm::length(velocity) < 1.0f * deltaTime) {
      GetMesh()->Play("Idle");
      SetState(State::idle);
    }
    break;
  case State::idle:
    CheckFall(deltaTime, groundHeight);
    break;
  case State::jump:
    if (isGrounded) {
      GetMesh()->Play("Idle");
      SetState(State::idle);
    } else if (CheckFall(deltaTime, groundHeight)) {
    }
    break;
  case State::guard:
    CheckFall(deltaTime, groundHeight);
    break;
  case State::lightAttack:
  case State::heavyAttack:
    stateTimer += deltaTime;
    if (stateTimer > 0.1f && stateTimer < 0.3f) {
      const glm::vec3 front = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 0, 1)) * glm::vec4(0, 0, -1, 1);
      colWorldAttack.center = position + front;
      colWorldAttack.r = 0.5f;
    }
    if (GetMesh()->IsFinished()) {
      GetMesh()->Play("Idle");
      SetState(State::idle);
      CheckFall(deltaTime, groundHeight);
    }
    break;
  case State::jumpAttack:
    if (isGrounded) {
      GetMesh()->Play("Idle");
      SetState(State::idle);
    } else if (GetMesh()->IsFinished()) {
      if (position.y - gravity > 0) {
        GetMesh()->Play("Jump");
      } else {
        GetMesh()->Play("Fall");
      }
      SetState(State::jump);
    }
    break;
  case State::damage:
    if (GetMesh()->IsFinished()) {
      GetMesh()->Play("Idle");
      SetState(State::idle);
    }
    break;
  case State::dead:
    break;
  }
}

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
  heightMap.Load("Res/HeightMap.tga", 100.0f, 0.5f);
  heightMap.CreateMesh(meshBuffer, "Terrain", "Res/ColorMap.tga");

  meshBuffer.LoadSkeletalMesh("Res/oni_small.gltf");
  meshBuffer.LoadSkeletalMesh("Res/oni_medium.gltf");
  meshBuffer.LoadSkeletalMesh("Res/bikuni.gltf");
  meshBuffer.LoadMesh("Res/weed_collection.gltf");
  meshBuffer.LoadMesh("Res/red_pine_tree.gltf");
  meshBuffer.LoadMesh("Res/farmers_house.gltf");
  meshBuffer.LoadMesh("Res/jizo_statue.gltf");
  meshBuffer.LoadMesh("Res/temple.gltf");
  meshBuffer.LoadMesh("Res/wood_well.gltf");
  meshBuffer.LoadSkeletalMesh("Res/effect_hit.gltf");

  terrain = std::make_shared<StaticMeshActor>(meshBuffer.GetMesh("Terrain"), "Terrain", 100, glm::vec3(0));

  glm::vec3 startPos(100, 0, 150);
  startPos.y = heightMap.Height(startPos);
  {
    player = std::make_shared<PlayerActor>(meshBuffer.GetSkeletalMesh("Bikuni"), "Player", 20, startPos);
    player->SetHeightMap(&heightMap);
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
      const std::vector<Mesh::Animation>& animList = mesh->GetAnimationList();
      mesh->Play(animList[i % animList.size()].name);
      SkeletalMeshActorPtr p = std::make_shared<SkeletalMeshActor>(mesh, "Kooni", 13, position, rotation, scale);
      p->colLocal = Collision::CreateSphere({ 0, 0.7f, 0 }, 0.5f);
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
* TODO: ジャンプ動作.
* TODO: 攻撃動作と攻撃判定.
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

  fontRenderer.BeginUpdate();
  if (IsActive()) {
    std::wstringstream wss;
    wss << L"FPS:" << std::fixed << std::setprecision(2) << window.Fps();
    fontRenderer.AddString(glm::vec2(-600, 300), wss.str().c_str());
    fontRenderer.AddString(glm::vec2(-600, 260), L"メインゲーム画面");
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
  effects.Update(deltaTime);

  DetectCollision(player, enemies, PlayerObstacleCollisionHandler);
  DetectCollision(player, trees, PlayerObstacleCollisionHandler);
  DetectCollision(player, buildings, PlayerObstacleCollisionHandler);

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
