/**
* @file Scene.h
*/
#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED
#include <vector>
#include <memory>
#include <string>
#include <iostream>

class SceneStack;

class Scene {
public:
  Scene(const char* name) : name(name) {
    std::cout << "Scene Constructor: " << name << "\n";
  }
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  virtual ~Scene() {
    Finalize();
    std::cout << "Scene Destructor: " << name << "\n";
  }
  virtual bool Initialize() = 0;
  virtual void Update(SceneStack&, float) = 0;
  virtual void Render() = 0;
  virtual void Finalize() = 0 {};

  const std::string& Name() const { return name; }

  virtual void Show() {
    isVisible = true;
    std::cout << "Scene Show: " << name << "\n";
  }
  virtual void Hide() {
    isVisible = false;
    std::cout << "Scene Hide: " << name << "\n";
  }
  bool IsVisible() const { return isVisible; }

  virtual void Activate() {
    isActive = true;
    std::cout << "Scene Activate: " << name << "\n";
  }
  virtual void Deactivate() {
    isActive = false;
    std::cout << "Scene Deactivate: " << name << "\n";
  }
  bool IsActive() const { return isActive; }

private:
  std::string name;
  bool isVisible = true;
  bool isActive = true;
};
using ScenePtr = std::shared_ptr<Scene>;

/**
*/
class SceneStack {
public:
  SceneStack() {
    stack.reserve(16);
  }
  SceneStack(const SceneStack&) = delete;
  SceneStack& operator=(const SceneStack&) = delete;
  ~SceneStack() = default;

  void Push(ScenePtr p) {
    if (!stack.empty()) {
      Current().Deactivate();
    }
    stack.push_back(p);
    Current().Initialize();
    Current().Activate();
    std::cout << "SceneStack Push: " << p->Name() << "\n";
  }
  void Pop() {
    if (!stack.empty()) {
      Current().Deactivate();
      Current().Finalize();
    }
    const std::string sceneName = stack.back()->Name();
    stack.pop_back();
    if (!stack.empty()) {
      Current().Activate();
    }
    std::cout << "SceneStack Pop: " << sceneName << "\n";
  }
  void Replace(ScenePtr p) {
    const std::string sceneName = stack.back()->Name();
    if (!stack.empty()) {
      Current().Deactivate();
      Current().Finalize();
      stack.pop_back();
    }
    stack.push_back(p);
    Current().Initialize();
    Current().Activate();
    std::cout << "SceneStack Replace: " << sceneName << " -> " << p->Name() << "\n";
  }
  Scene& Current() { return *stack.back(); }
  const Scene& Current() const { return *stack.back(); }
  size_t Size() const { return stack.size(); }
  bool Empty() const { return stack.empty(); }

  void Update(float deltaTime) {
    for (auto& e : stack) {
      e->Update(*this, deltaTime);
    }
  }
  void Render() {
    for (auto& e : stack) {
      if (e->IsVisible()) {
        e->Render();
      }
    }
  }

private:
  std::vector<ScenePtr> stack;
};

#endif // SCENE_H_INCLUDED