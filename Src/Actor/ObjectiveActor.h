/**
* @file ObjectiveActor.h
*/
#ifndef OBJECTIVEACTOR_H_INCLUDED
#define OBJECTIVEACTOR_H_INCLUDED
#include "../Actor.h"

/**
*
*/
class ObjectiveActor : public StaticMeshActor
{
public:
  ObjectiveActor(const Mesh::MeshPtr& m, const std::string& name, int hp, const glm::vec3& pos,
    const glm::vec3& rot = glm::vec3(0), const glm::vec3& scale = glm::vec3(1))
    : StaticMeshActor(m, name, hp, pos, rot, scale)
  {
    colLocal = Collision::CreateCapsule(glm::vec3(0, -1, 0), glm::vec3(0, 1, 0), 0.5f);
    color = glm::vec4(1, 0.5f, 0.25f, 1);
  }
  virtual ~ObjectiveActor() = default;

  virtual void OnHit(const ActorPtr& other, const glm::vec3& p) override {
    if (!isAchieved) {
      color = glm::vec4(1, 1, 1, 1);
      isAchieved = true;
    }
  }

  bool IsAchieved() const { return isAchieved; }

private:
  bool isAchieved = false;
};
using ObjectiveActorPtr = std::shared_ptr<ObjectiveActor>;

#endif // OBJECTIVEACTOR_H_INCLUDED