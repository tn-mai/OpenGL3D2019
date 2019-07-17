/**
* @file PlayerActor.h
*/
#ifndef PLAYERACTOR_H_INCLUDED
#define PLAYERACTOR_H_INCLUDED
#include "../Actor.h"
#include "../GLFWEW.h"
#include "../Terrain.h"

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
  virtual void UpdateDrawData(float) override;
  virtual void Draw() override;
  void ProcessInput();
  void SetHeightMap(const Terrain::HeightMap* p) { heightMap = p; }
  const ActorPtr& GetAttackCollision() const { return attackCollision; }
  void ClearAttackCollision() { attackCollision.reset(); }
  void SetMeshBuffer(Mesh::Buffer* b) { buffer = b; }
  void BoardToObject(const ActorPtr& obj) {
    bordingObject = obj;
    velocity.y = 0;
    onObject = true;
  }
  void DisembarkFromObject() {
    bordingObject.reset();
    onObject = false;
  }

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
  void SetState(State s);

  State state = State::idle;
  float stateTimer = 0;
  bool isGrounded = false;
  bool onObject = false;
  ActorPtr bordingObject;
  ActorPtr attackCollision;
  const Terrain::HeightMap* heightMap = nullptr;
  Mesh::Buffer* buffer = nullptr;
};
using PlayerActorPtr = std::shared_ptr<PlayerActor>;

#endif // PLAYERACTOR_H_INCLUDED