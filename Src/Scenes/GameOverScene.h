/**
* @file GameOverScene.h
*/
#ifndef GAMEOVERSCENE_H_INCLUDED
#define GAMEOVERSCENE_H_INCLUDED
#include "../Scene.h"

/**
* ゲームオーバー画面.
*/
class GameOverScene : public Scene
{
public:
  GameOverScene();
  virtual bool Initialize() override;
  virtual void Update(SceneStack&, float) override;
  virtual void Render() override;
  virtual void Finalize() override;
};

#endif // GAMEOVERSCENE_H_INCLUDED