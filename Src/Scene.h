/**
* @file Scene.h
*/
#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED
#include <vector>
#include <memory>
#include <string>

class SceneStack;

/**
* シーンの基底クラス.
*/
class Scene {
public:
  Scene(const char* name);
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  virtual ~Scene();

  virtual bool Initialize() = 0 {}
  virtual void ProcessInput(SceneStack&, float) {}
  virtual void Update(SceneStack&, float) = 0 {}
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
* シーン管理クラス.
*/
class SceneStack {
public:
  SceneStack();
  SceneStack(const SceneStack&) = delete;
  SceneStack& operator=(const SceneStack&) = delete;
  ~SceneStack() = default;

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
  std::vector<ScenePtr> stack;
};

#endif // SCENE_H_INCLUDED