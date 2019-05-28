/**
* @file GameOverScene.h
*/
#ifndef GAMEOVERSCENE_H_INCLUDED
#define GAMEOVERSCENE_H_INCLUDED
#include "../Scene.h"
#include "../Font.h"

/**
* ゲームオーバー画面.
*/
class GameOverScene : public Scene
{
public:
  GameOverScene();
  virtual bool Initialize() override;
  virtual void ProcessInput() override;
  virtual void Update(float) override;
  virtual void Render() override;
  virtual void Finalize() override;
private:
  Font::Renderer fontRenderer;
};

#endif // GAMEOVERSCENE_H_INCLUDED