/**
* @file TitleScene.h
*/
#ifndef TITLESCENE_H_INCLUDED
#define TITLESCENE_H_INCLUDED
#include "../Scene.h"
#include "../Texture.h"
#include "../BufferObject.h"
#include "../Shader.h"
#include <memory>

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
  std::unique_ptr<Texture::Image2D> tex;
  BufferObject vbo;
  BufferObject ibo;
  VertexArrayObject vao;
  Shader::ProgramPtr program;
};

#endif // TITLESCENE_H_INCLUDED
