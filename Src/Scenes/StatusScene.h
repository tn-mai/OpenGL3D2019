/**
* @file StatusScene.h
*/
#ifndef STATUSSCENE_H_INCLUDED
#define STATUSSCENE_H_INCLUDED
#include "../Scene.h"

/**
* ステータス画面.
*/
class StatusScene : public Scene
{
public:
  StatusScene();
  virtual bool Initialize() override;
  virtual void Update(SceneStack&, float) override;
  virtual void Render() override;
  virtual void Finalize() override;
};

#endif // STATUSSCENE_H_INCLUDED
