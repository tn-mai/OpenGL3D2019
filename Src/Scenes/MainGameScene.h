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
  virtual void Update(SceneStack&, float) override;
  virtual void Render() override;
  virtual void Finalize() override;
  virtual void Play() override;

private:
  Font::Renderer fontRenderer;
  Mesh::Buffer meshBuffer;
  Shader::ProgramPtr progMesh;
  Terrain::HeightMap heightMap;
  Texture::Image2DPtr texTerrain;
  glm::vec3 pos = glm::vec3(100, 10, 100);
  glm::vec3 dir = glm::vec3(0, 0, -1);
  glm::vec2 prevMousePos = glm::vec2(0, 0);
};

#endif // MAINGAMESCENE_H_INCLUDED
