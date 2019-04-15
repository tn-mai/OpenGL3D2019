/**
* @file TitleScene.h
*/
#ifndef TITLESCENE_H_INCLUDED
#define TITLESCENE_H_INCLUDED
#include "../Scene.h"
#include "../Sprite.h"
#include "../Font.h"
#include "../Mesh.h"
#include "../Terrain.h"
#include <vector>

/**
* ƒ^ƒCƒgƒ‹‰æ–Ê.
*/
class TitleScene : public Scene
{
public:
  TitleScene();
  virtual bool Initialize() override;
  virtual void Update(SceneStack&, float) override;
  virtual void Render() override;
  virtual void Finalize() override;
private:
  std::vector<Sprite> sprites;
  SpriteRenderer spriteRenderer;
  Font::Renderer fontRenderer;
  Mesh::Buffer meshBuffer;
  Shader::ProgramPtr progMesh;
  Terrain::HeightMap heightMap;
  Texture::Image2DPtr texTerrain;
  glm::vec3 pos = glm::vec3(100, 100, 100);
  glm::vec3 dir = glm::vec3(0, 0, -1);
  glm::vec2 prevMousePos = glm::vec2(0, 0);
};

#endif // TITLESCENE_H_INCLUDED
