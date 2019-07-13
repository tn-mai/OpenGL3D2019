/**
* @file Actor.cpp
*/
#include "Actor.h"
#include "SkeletalMesh.h"
#include <glm/gtc/matrix_transform.hpp>

/**
* コンストラクタ.
*
* @param name     アクターの名前.
* @param health   耐久力.
* @param position 位置.
* @param rotation 回転.
* @param scale    拡大率.
*
* 指定された名前、耐久力、位置、回転、拡大率によってアクターを初期化する.
*/
Actor::Actor(const std::string& name, int health,
  const glm::vec3& position, const glm::vec3& rotation,
  const glm::vec3& scale)
  : name(name), health(health), position(position),
  rotation(rotation), scale(scale)
{
}

/**
* 更新.
*
* @param deltaTime 経過時間.
*
* UpdateDrawData()より前に実行すること.
*/
void Actor::Update(float deltaTime)
{
  position += velocity * deltaTime;

  colWorld = colLocal;
  switch (colWorld.type) {
  case Collision::Shape::Type::sphere:
    colWorld.s.center += position;
    break;

  case Collision::Shape::Type::capsule:
    colWorld.c.line.a += position;
    colWorld.c.line.b += position;
    break;

  case Collision::Shape::Type::obb: {
    const glm::mat4 matR_Y = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0));
    const glm::mat4 matR_ZY = glm::rotate(matR_Y, rotation.z, glm::vec3(0, 0, -1));
    const glm::mat4 matR_XZY = glm::rotate(matR_ZY, rotation.x, glm::vec3(1, 0, 0));
    colWorld.obb.center += position;
    colWorld.obb.e *= scale;
    for (glm::vec3& e : colWorld.obb.axis) {
      e = matR_XZY * glm::vec4(e, 1);
    }
    break;
  }
  }
}

/**
* 描画情報の更新.
*
* @param deltaTime 経過時間.
*
* Update()の後で実行すること.
*/
void Actor::UpdateDrawData(float deltaTime)
{
}

/**
* 描画.
*/
void Actor::Draw()
{
}

/**
* コンストラクタ.
*
* @param m        表示するメッシュ.
* @param name     アクターの名前.
* @param health   耐久力.
* @param position 位置.
* @param rotation 回転.
* @param scale    拡大率.
*
* 指定されたメッシュ、名前、耐久力、位置、回転、拡大率によってアクターを初期化する.
*/
StaticMeshActor::StaticMeshActor(const Mesh::MeshPtr& m,
  const std::string& name, int health, const glm::vec3& position,
  const glm::vec3& rotation, const glm::vec3& scale)
  : Actor(name, health, position, rotation, scale), mesh(m)
{
}

/**
* 描画.
*/
void StaticMeshActor::Draw()
{
  if (mesh) {
    const glm::mat4 matT = glm::translate(glm::mat4(1), position);
    const glm::mat4 matR_Y = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0));
    const glm::mat4 matR_ZY = glm::rotate(matR_Y, rotation.z, glm::vec3(0, 0, -1));
    const glm::mat4 matR_XZY = glm::rotate(matR_ZY, rotation.x, glm::vec3(1, 0, 0));
    const glm::mat4 matS = glm::scale(glm::mat4(1), scale);
    const glm::mat4 matModel = matT * matR_XZY * matS;
    mesh->Draw(matModel);
  }
}

/**
* コンストラクタ.
*
* @param m        表示するメッシュ.
* @param name     アクターの名前.
* @param health   耐久力.
* @param position 位置.
* @param rotation 回転.
* @param scale    拡大率.
*
* 指定されたメッシュ、名前、耐久力、位置、回転、拡大率によってアクターを初期化する.
*/
SkeletalMeshActor::SkeletalMeshActor(const Mesh::SkeletalMeshPtr& m,
  const std::string& name, int health, const glm::vec3& position,
  const glm::vec3& rotation, const glm::vec3& scale)
  : Actor(name, health, position, rotation, scale), mesh(m)
{
}

/**
* 描画情報の更新.
*
* @param deltaTime 経過時間.
*/
void SkeletalMeshActor::UpdateDrawData(float deltaTime)
{
  if (mesh) {
    const glm::mat4 matT = glm::translate(glm::mat4(1), position);
    const glm::mat4 matR_Y = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0));
    const glm::mat4 matR_ZY = glm::rotate(matR_Y, rotation.z, glm::vec3(0, 0, -1));
    const glm::mat4 matR_XZY = glm::rotate(matR_ZY, rotation.x, glm::vec3(1, 0, 0));
    const glm::mat4 matS = glm::scale(glm::mat4(1), scale);
    const glm::mat4 matModel = matT * matR_XZY * matS;
    mesh->Update(deltaTime, matModel, glm::vec4(1));
  }
}

/**
* 描画.
*/
void SkeletalMeshActor::Draw()
{
  if (mesh) {
    mesh->Draw();
  }
}

/**
* 格納可能なアクター数を確保する.
*
* @param reserveCount アクター配列の確保数.
*/
void ActorList::Reserve(size_t reserveCount)
{
  actors.reserve(reserveCount);
}

/**
* アクターを追加する.
*
* @param actor 追加するアクター.
*/
void ActorList::Add(const ActorPtr& actor)
{
  actors.push_back(actor);
}

/**
* アクターを削除する.
*
* @param actor 削除するアクター.
*/
bool ActorList::Remove(const ActorPtr& actor)
{
  auto itr = std::find(actors.begin(), actors.end(), actor);
  if (itr != actors.end()) {
    actors.erase(itr);
    return true;
  }
  return false;
}

/**
* アクターリストを空にする.
*/
void ActorList::Clear()
{
  actors.clear();
}

/**
* アクターの状態を更新する.
*
* @param deltaTime 前回の更新からの経過時間.
*/
void ActorList::Update(float deltaTime)
{
  for (const ActorPtr& e : actors) {
    if (e && e->health > 0) {
      e->Update(deltaTime);
    }
  }
}

/**
* アクターの描画データを更新する.
*
* @param deltaTime 前回の更新からの経過時間.
*/
void ActorList::UpdateDrawData(float deltaTime)
{
  for (const ActorPtr& e : actors) {
    if (e && e->health > 0) {
      e->UpdateDrawData(deltaTime);
    }
  }
}

/**
* Actorを描画する.
*/
void ActorList::Draw()
{
  for (const ActorPtr& e : actors) {
    if (e && e->health > 0) {
      e->Draw();
    }
  }
}

/**
* 名前でアクターを検索する.
*
* @param name 検索するアクター名.
*
* @return 名前がnameと一致するアクター.
*         一致するアクターがいなければnullptrを返す.
*/
ActorPtr ActorList::Find(const std::string& name) const
{
  for (const ActorPtr& e : actors) {
    if (e->name == name) {
      return e;
    }
  }
  return nullptr;
}

/**
* 衝突判定を行う.
*
* @param a       判定対象のアクターその１.
* @param b       判定対象のアクターその２.
* @param handler 衝突した場合に実行される関数.
*
* @retval true  衝突した.
* @retval false 衝突しなかった.
*/
bool DetectCollision(const ActorPtr& a, const ActorPtr& b, CollsionHandlerType handler)
{
  if (a->colWorld.type == Collision::Shape::Type::sphere) {
    Collision::Sphere& colA = a->colWorld.s;
    if (b->colWorld.type == Collision::Shape::Type::sphere) {
      if (Collision::TestSphereSphere(colA, b->colWorld.s)) {
        handler(a, b, b->colWorld.s.center);
        return true;
      }
    } else if (b->colWorld.type == Collision::Shape::Type::capsule) {
      glm::vec3 p;
      if (Collision::TestSphereCapsule(colA, b->colWorld.c, &p)) {
        handler(a, b, p);
        return true;
      }
    } else if (b->colWorld.type == Collision::Shape::Type::obb) {
      glm::vec3 p;
      if (Collision::TestSphereOBB(colA, b->colWorld.obb, &p)) {
        handler(a, b, p);
        return true;
      }
    }
  }
  return false;
}

/**
* 衝突判定を行う.
*
* @param a       判定対象のアクター.
* @param b       判定対象のアクターリスト.
* @param handler 衝突した場合に実行される関数.
*
* @retval true  衝突によってaのhealthが0以下になった.
* @retval false aのhealthが最初から0以下、あるいは
*/
void DetectCollision(const ActorPtr& a, const ActorList& b, CollsionHandlerType handler)
{
  if (a->health <= 0) {
    return;
  }
  for (const ActorPtr& actorB : b) {
    if (actorB->health <= 0) {
      continue;
    }
    if (DetectCollision(a, actorB, handler)) {
      if (a->health <= 0) {
        break;
      }
    }
  }
}

/**
* 衝突判定を行う.
*
* @param a       判定対象のアクターリストその１.
* @param b       判定対象のアクターリストその２.
* @param handler 衝突した場合に実行される関数.
*/
void DetectCollision(const ActorList& a, const ActorList& b, CollsionHandlerType handler)
{
  for (const ActorPtr& actorA : a) {
    DetectCollision(actorA, b, handler);
  }
}

#if 0
/**
* 2つの長方形の衝突状態を調べる.
*
* @param lhs 長方形その1.
* @param rhs 長方形その2.
*
* @retval true  衝突している.
* @retval false 衝突していない.
*/
bool DetectCollision(const Actor& lhs, const Actor& rhs)
{
  return
    lhs.colWorld.origin.x < rhs.colWorld.origin.x + rhs.colWorld.size.x &&
    lhs.colWorld.origin.x + lhs.colWorld.size.x > rhs.colWorld.origin.x &&
    lhs.colWorld.origin.y < rhs.colWorld.origin.y + rhs.colWorld.size.y &&
    lhs.colWorld.origin.y + lhs.colWorld.size.y > rhs.colWorld.origin.y &&
    lhs.colWorld.origin.z < rhs.colWorld.origin.z + rhs.colWorld.size.z &&
    lhs.colWorld.origin.z + lhs.colWorld.size.z > rhs.colWorld.origin.z;
}

/**
* 2つのグループ間で衝突判定を行う.
*
* @param va グループA.
* @param vb グループB.
* @param func A-B間の衝突を処理する関数. 
*/
void DetectCollision(std::vector<Actor*>& va, std::vector<Actor*>& vb, CollsionHandlerType func)
{
  for (auto& a : va) {
    if (a->health <= 0) {
      continue;
    }
    for (auto& b : vb) {
      if (b->health <= 0) {
        continue;
      }
      if (DetectCollision(*a, *b)) {
        func(*a, *b);
        if (a->health <= 0) {
          break;
        }
      }
    }
  }
}

#endif