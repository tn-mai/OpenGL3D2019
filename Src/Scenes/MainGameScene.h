/**
* @file MainGameScene.h
*/
#ifndef MAINGAMESCENE_H_INCLUDED
#define MAINGAMESCENE_H_INCLUDED
#include "../Scene.h"
#include "../Actor.h"
#include "../SkeletalMesh.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Font.h"
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

private:
  enum class State {
    idle,
    run,
    jump,
    attack,
    jumpAttack,
    guard,
    damage,
    dead,
  };
  State state = State::idle;
  float stateTimer = 0;
  bool isGrounded = false;
  Collision::Sphere  colWorldAttack;
};

/**
* メインゲーム画面.
*/
class MainGameScene : public Scene
{
public:
  MainGameScene();
  virtual bool Initialize() override;
  virtual void ProcessInput() override;
  virtual void Update(float) override;
  virtual void Render() override;
  virtual void Finalize() override;
  virtual void Play() override;

private:
  Font::Renderer fontRenderer;
  Mesh::Buffer meshBuffer;
  SkeletalMeshActorPtr player;
  StaticMeshActorPtr terrain;
  ActorList trees;
  ActorList vegetations;
  ActorList buildings;
  ActorList enemies;

  Terrain::HeightMap heightMap;

  float actionWaitTimer = 0;
  glm::vec3 pos = glm::vec3(0, 10, 10);
  glm::vec3 dir = glm::vec3(0, 0, -1);
};

#endif // MAINGAMESCENE_H_INCLUDED
