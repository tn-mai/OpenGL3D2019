/**
* @file Actor.cpp
*/
#include "Actor.h"
#include "SkeletalMesh.h"
#include <glm/gtc/matrix_transform.hpp>

/**
* �R���X�g���N�^.
*
* @param name     �A�N�^�[�̖��O.
* @param health   �ϋv��.
* @param position �ʒu.
* @param rotation ��].
* @param scale    �g�嗦.
*
* �w�肳�ꂽ���O�A�ϋv�́A�ʒu�A��]�A�g�嗦�ɂ���ăA�N�^�[������������.
*/
Actor::Actor(const std::string& name, int health,
  const glm::vec3& position, const glm::vec3& rotation,
  const glm::vec3& scale)
  : name(name), health(health), position(position),
  rotation(rotation), scale(scale)
{
}

/**
* �X�V.
*
* @param deltaTime �o�ߎ���.
*
* UpdateDrawData()���O�Ɏ��s���邱��.
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
* �`����̍X�V.
*
* @param deltaTime �o�ߎ���.
*
* Update()�̌�Ŏ��s���邱��.
*/
void Actor::UpdateDrawData(float deltaTime)
{
}

/**
* �`��.
*/
void Actor::Draw()
{
}

/**
* �R���X�g���N�^.
*
* @param m        �\�����郁�b�V��.
* @param name     �A�N�^�[�̖��O.
* @param health   �ϋv��.
* @param position �ʒu.
* @param rotation ��].
* @param scale    �g�嗦.
*
* �w�肳�ꂽ���b�V���A���O�A�ϋv�́A�ʒu�A��]�A�g�嗦�ɂ���ăA�N�^�[������������.
*/
StaticMeshActor::StaticMeshActor(const Mesh::MeshPtr& m,
  const std::string& name, int health, const glm::vec3& position,
  const glm::vec3& rotation, const glm::vec3& scale)
  : Actor(name, health, position, rotation, scale), mesh(m)
{
}

/**
* �`��.
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
* �R���X�g���N�^.
*
* @param m        �\�����郁�b�V��.
* @param name     �A�N�^�[�̖��O.
* @param health   �ϋv��.
* @param position �ʒu.
* @param rotation ��].
* @param scale    �g�嗦.
*
* �w�肳�ꂽ���b�V���A���O�A�ϋv�́A�ʒu�A��]�A�g�嗦�ɂ���ăA�N�^�[������������.
*/
SkeletalMeshActor::SkeletalMeshActor(const Mesh::SkeletalMeshPtr& m,
  const std::string& name, int health, const glm::vec3& position,
  const glm::vec3& rotation, const glm::vec3& scale)
  : Actor(name, health, position, rotation, scale), mesh(m)
{
}

/**
* �`����̍X�V.
*
* @param deltaTime �o�ߎ���.
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
* �`��.
*/
void SkeletalMeshActor::Draw()
{
  if (mesh) {
    mesh->Draw();
  }
}

/**
* �i�[�\�ȃA�N�^�[�����m�ۂ���.
*
* @param reserveCount �A�N�^�[�z��̊m�ې�.
*/
void ActorList::Reserve(size_t reserveCount)
{
  actors.reserve(reserveCount);
}

/**
* �A�N�^�[��ǉ�����.
*
* @param actor �ǉ�����A�N�^�[.
*/
void ActorList::Add(const ActorPtr& actor)
{
  actors.push_back(actor);
}

/**
* �A�N�^�[���폜����.
*
* @param actor �폜����A�N�^�[.
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
* �A�N�^�[���X�g����ɂ���.
*/
void ActorList::Clear()
{
  actors.clear();
}

/**
* �A�N�^�[�̏�Ԃ��X�V����.
*
* @param deltaTime �O��̍X�V����̌o�ߎ���.
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
* �A�N�^�[�̕`��f�[�^���X�V����.
*
* @param deltaTime �O��̍X�V����̌o�ߎ���.
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
* Actor��`�悷��.
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
* ���O�ŃA�N�^�[����������.
*
* @param name ��������A�N�^�[��.
*
* @return ���O��name�ƈ�v����A�N�^�[.
*         ��v����A�N�^�[�����Ȃ����nullptr��Ԃ�.
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
* �Փ˔�����s��.
*
* @param a       ����Ώۂ̃A�N�^�[���̂P.
* @param b       ����Ώۂ̃A�N�^�[���̂Q.
* @param handler �Փ˂����ꍇ�Ɏ��s�����֐�.
*
* @retval true  �Փ˂���.
* @retval false �Փ˂��Ȃ�����.
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
* �Փ˔�����s��.
*
* @param a       ����Ώۂ̃A�N�^�[.
* @param b       ����Ώۂ̃A�N�^�[���X�g.
* @param handler �Փ˂����ꍇ�Ɏ��s�����֐�.
*
* @retval true  �Փ˂ɂ����a��health��0�ȉ��ɂȂ���.
* @retval false a��health���ŏ�����0�ȉ��A���邢��
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
* �Փ˔�����s��.
*
* @param a       ����Ώۂ̃A�N�^�[���X�g���̂P.
* @param b       ����Ώۂ̃A�N�^�[���X�g���̂Q.
* @param handler �Փ˂����ꍇ�Ɏ��s�����֐�.
*/
void DetectCollision(const ActorList& a, const ActorList& b, CollsionHandlerType handler)
{
  for (const ActorPtr& actorA : a) {
    DetectCollision(actorA, b, handler);
  }
}

#if 0
/**
* 2�̒����`�̏Փˏ�Ԃ𒲂ׂ�.
*
* @param lhs �����`����1.
* @param rhs �����`����2.
*
* @retval true  �Փ˂��Ă���.
* @retval false �Փ˂��Ă��Ȃ�.
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
* 2�̃O���[�v�ԂŏՓ˔�����s��.
*
* @param va �O���[�vA.
* @param vb �O���[�vB.
* @param func A-B�Ԃ̏Փ˂���������֐�. 
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