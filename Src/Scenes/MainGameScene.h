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

class PlayerActor;

/**
* ÉÅÉCÉìÉQÅ[ÉÄâÊñ .
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
  std::shared_ptr<PlayerActor> player;
  StaticMeshActorPtr terrain;
  ActorList trees;
  ActorList vegetations;
  ActorList buildings;
  ActorList enemies;
  ActorList effects;

  Terrain::HeightMap heightMap;

  float actionWaitTimer = 0;
  glm::vec3 pos = glm::vec3(0, 10, 10);
  glm::vec3 dir = glm::vec3(0, 0, -1);
};

#endif // MAINGAMESCENE_H_INCLUDED
