/**
* @file TitleScene.h
*/
#ifndef TITLESCENE_H_INCLUDED
#define TITLESCENE_H_INCLUDED
#include "../Scene.h"

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
};

#endif // TITLESCENE_H_INCLUDED
