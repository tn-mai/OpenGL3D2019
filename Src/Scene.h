/**
* @file Scene.h
*/
#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED
#include <vector>
#include <memory>
#include <string>
#include "Sprite.h"

class SceneStack;

/**
* �V�[���̊��N���X.
*/
class Scene {
public:
  Scene(const char* name);
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  virtual ~Scene();

  virtual bool Initialize() = 0 {}
  virtual void ProcessInput() {}
  virtual void Update(float) = 0 {}
  virtual void Render() = 0 {}
  virtual void Finalize() = 0 {}

  const std::string& Name() const;

  virtual void Show();
  virtual void Hide();
  bool IsVisible() const;
  virtual void Play();
  virtual void Stop();
  bool IsActive() const;

private:
  std::string name;
  bool isVisible = true;
  bool isActive = true;
};
using ScenePtr = std::shared_ptr<Scene>;

/**
* �V�[���Ǘ��N���X.
*/
class SceneStack {
public:
  static SceneStack& Instance();

  void Push(ScenePtr p);
  void Pop();
  void Replace(ScenePtr p);
  Scene& Current();
  const Scene& Current() const;
  size_t Size() const;
  bool Empty() const;

  void Update(float deltaTime);
  void Render();

private:
  SceneStack();
  SceneStack(const SceneStack&) = delete;
  SceneStack& operator=(const SceneStack&) = delete;
  ~SceneStack() = default;

  std::vector<ScenePtr> stack;
};

/**
* �V�[���E�t�F�[�_�[.
*
* �t�F�[�h�C���E�t�F�[�h�A�E�g�𐧌䂷��N���X.
*/
class SceneFader
{
public:
  enum class Mode {
    none, ///< �t�F�[�h�C���E�t�F�[�h�A�E�g�����Ă��Ȃ�(�I�����Ă���).
    fadeIn, ///< �t�F�[�h�C����.
    fadeOut, ///< �t�F�[�h�A�E�g��.
  };

  static SceneFader& Instance();
  void Update(float deltaTime);
  void Render() const;

  void FadeOut(float time, const glm::vec3& color = glm::vec3(0));
  void FadeIn(float time);
  bool IsFading() const;

private:
  SceneFader();
  SceneFader(const SceneFader&) = delete;
  SceneFader& operator=(const SceneFader&) = delete;
  ~SceneFader() = default;

  Mode mode = Mode::none;
  float totalTime = 0;
  float timer = 0;
  SpriteRenderer spriteRenderer;
  Sprite spr;
};

#endif // SCENE_H_INCLUDED