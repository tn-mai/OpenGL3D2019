/**
* @file PlayerActor.cpp
*/
#include "PlayerActor.h"
#include "../SkeletalMesh.h"
#include <iostream>

/**
*
*/
void PlayerActor::SetState(State s)
{
  static const char* const names[] = {
    "idle", "run", "jump", "lightAttack", "heavyAttack", "jumpAttack", "guard", "damage", "dead"
  };
  if (state != s) {
    std::cout << "[INFO]" << __func__ << ":" << names[static_cast<int>(state)] << "->" << names[static_cast<int>(s)] << "\n";
    state = s;
  }
}

/**
* コンストラクタ.
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
  if ((isGrounded || onObject) && (gamepad.buttonDown & GamePad::B)) {
    GetMesh()->Play("Jump");
    static const float jumpVelocity = 5.0f;
    velocity.y = jumpVelocity;
    isGrounded = false;
    onObject = false;
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
  if (!onObject && (position.y - groundHeight >= 0.1f) && (velocity.y < -0.1f)) {
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
  if (isGrounded || onObject) {
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
    if (isGrounded || onObject) {
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
    if (stateTimer > 0.1f && stateTimer < 1.0f) {
      if (!attackCollision) {
        static const float radian = 0.5f;
        const glm::vec3 front = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 1.5f, 1);
        attackCollision = std::make_shared<StaticMeshActor>(buffer->GetMesh("Sphere"), "PlayerAttackCollision", 5, position + front + glm::vec3(0, 1, 0), glm::vec3(0), glm::vec3(radian));
        attackCollision->colLocal = Collision::CreateSphere(glm::vec3(0), radian);
      }
    } else {
      ClearAttackCollision();
    }
    if (GetMesh()->IsFinished()) {
      stateTimer = 0;
      GetMesh()->Play("Idle");
      SetState(State::idle);
      ClearAttackCollision();
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

  if (attackCollision) {
    attackCollision->Update(deltaTime);
  }
}

/**
*
*/
void PlayerActor::UpdateDrawData(float deltaTime)
{
  SkeletalMeshActor::UpdateDrawData(deltaTime);
  if (attackCollision) {
    attackCollision->UpdateDrawData(deltaTime);
  }
}

/**
*
*/
void PlayerActor::Draw()
{
  SkeletalMeshActor::Draw();
  if (attackCollision) {
    attackCollision->Draw();
  }
}
