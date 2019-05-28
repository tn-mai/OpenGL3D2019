/**
* @file Scene.cpp
*/
#include "Scene.h"
#include <iostream>

/**
* コンストラクタ.
*
* @param name シーン名.
*/
Scene::Scene(const char* name) : name(name)
{
  std::cout << "Scene Constructor: " << name << "\n";
}

/**
* デストラクタ.
*/
Scene::~Scene()
{
  Finalize();
  std::cout << "Scene Destructor: " << name << "\n";
}

/**
* シーンを活動状態にする.
*/
void Scene::Play()
{
  isActive = true;
  std::cout << "Scene Play: " << name << "\n";
}

/**
* シーンを停止状態にする.
*/
void Scene::Stop()
{
  isActive = false;
  std::cout << "Scene Stop: " << name << "\n";
}

/**
* シーンを表示する.
*/
void Scene::Show()
{
  isVisible = true;
  std::cout << "Scene Show: " << name << "\n";
}

/**
* シーンを非表示にする.
*/
void Scene::Hide()
{
  isVisible = false;
  std::cout << "Scene Hide: " << name << "\n";
}

/**
* シーン名を取得する.
*
* @return シーン名.
*/
const std::string& Scene::Name() const
{
  return name;
}

/**
* シーンの活動状態を調べる.
*
* @retval true  活動している.
* @retval false 停止している.
*/
bool Scene::IsActive() const
{
  return isActive;
}

/**
* シーンの表示状態を調べる.
*
* @retval true  表示状態.
* @retval false 非表示状態.
*/
bool Scene::IsVisible() const
{
  return isVisible;
}

/**
* シーンスタックのシングルトンインスタンスを取得する.
*
* @return シーンスタックのシングルトンインスタンス.
*/
SceneStack& SceneStack::Instance()
{
  static SceneStack instance;
  return instance;
}

/**
* コンストラクタ.
*/
SceneStack::SceneStack()
{
  stack.reserve(16);
}

/**
* シーンをプッシュする.
*
* @param p 新しいシーン.
*/
void SceneStack::Push(ScenePtr p)
{
  if (!stack.empty()) {
    Current().Stop();
  }
  stack.push_back(p);
  std::cout << "SceneStack Push: " << p->Name() << "\n";
  Current().Initialize();
  Current().Play();
}

/**
* シーンをポップする.
*/
void SceneStack::Pop()
{
  if (stack.empty()) {
    std::cout << "SceneStack Pop: [警告]シーンスタックが空です.\n";
    return;
  }
  Current().Stop();
  Current().Finalize();
  const std::string sceneName = Current().Name();
  stack.pop_back();
  std::cout << "SceneStack Pop: " << sceneName << "\n";
  if (!stack.empty()) {
    Current().Play();
  }
}

/**
* シーンを置き換える.
*
* @param p 新しいシーン.
*/
void SceneStack::Replace(ScenePtr p)
{
  std::string sceneName = "(Empty)";
  if (stack.empty()) {
    std::cout << "SceneStack Replace: [警告]シーンスタックが空です.\n";
  } else {
    sceneName = Current().Name();
    Current().Stop();
    Current().Finalize();
    stack.pop_back();
  }
  stack.push_back(p);
  std::cout << "SceneStack Replace: " << sceneName << " -> " << p->Name() << "\n";
  Current().Initialize();
  Current().Play();
}

/**
* 現在のシーンを取得する.
*
* @return 現在のシーン.
*/
Scene& SceneStack::Current()
{
  return *stack.back();
}

/**
* 現在のシーンを取得する.
*
* @return 現在のシーン.
*/
const Scene& SceneStack::Current() const
{
  return *stack.back();
}

/**
* シーンの数を取得する.
*
* @return スタックに積まれているシーンの数.
*/
size_t SceneStack::Size() const
{
  return stack.size();
}

/**
* スタックが空かどうかを調べる.
*
* @retval true  スタックは空.
* @retval false スタックに1つ以上のシーンが積まれている.
*/
bool SceneStack::Empty() const
{
  return stack.empty();
}

/**
* シーンを更新する.
*
* @param deltaTime 前回の更新からの経過時間(秒).
*/
void SceneStack::Update(float deltaTime)
{
  if (!Empty()) {
    Current().ProcessInput();
  }
  for (auto& e : stack) {
    e->Update(deltaTime);
  }
}

/**
* シーンを描画する.
*/
void SceneStack::Render()
{
  for (auto& e : stack) {
    if (e->IsVisible()) {
      e->Render();
    }
  }
}