/**
* @file MainGameScene.h
*/
#ifndef MAINGAMESCENE_H_INCLUDED
#define MAINGAMESCENE_H_INCLUDED
#include "../Scene.h"
#include "../Mesh.h"
#include "../Texture.h"
#include "../Shader.h"
#include "../Font.h"
#include "../Terrain.h"

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
  Mesh::MeshPtr meshPlayer;
  Mesh::MeshPtr meshTerrain;
  Mesh::MeshPtr meshCircle;
  std::vector<Mesh::MeshPtr> meshTrees;
  std::vector<Mesh::MeshPtr> meshEnemies;
  Mesh::MeshPtr meshFarmersHouse;
  std::vector<Mesh::MeshPtr> meshWeeds;

  Shader::ProgramPtr progMesh;
  Shader::ProgramPtr progSkeletalMesh;

  Terrain::HeightMap heightMap;

  Texture::Image2DPtr texTerrain;
  Texture::Image2DPtr texTree;
  Texture::Image2DPtr texOniSmall;
  Texture::Image2DPtr texPlayer;
  Texture::Image2DPtr texFarmersHouse;
  Texture::Image2DPtr texPineTree;
  Texture::Image2DPtr texWeed;

  float actionWaitTimer = 0;
  glm::vec3 pos = glm::vec3(0, 10, 10);
  glm::vec3 dir = glm::vec3(0, 0, -1);
  glm::vec2 prevMousePos = glm::vec2(0, 0);
};

#endif // MAINGAMESCENE_H_INCLUDED
