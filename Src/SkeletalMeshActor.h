/**
* @file SkeletalMeshActor.h
*/
#ifndef SKELETALMESHACTOR_H_INCLUDED
#define SKELETALMESHACTOR_H_INCLUDED
#include "Actor.h"
#include "SkeletalMesh.h"

/**
* スケルタルメッシュ用アクター.
*/
class SkeletalMeshActor : public Actor
{
public:
  SkeletalMeshActor(const Mesh::SkeletalMeshPtr& m, const std::string& name, int hp, const glm::vec3& pos,
    const glm::vec3& rot = glm::vec3(0), const glm::vec3& scale = glm::vec3(1));
  virtual ~SkeletalMeshActor() = default;

  virtual void UpdateDrawData(float) override;
  virtual void Draw() override;

  const Mesh::SkeletalMeshPtr& GetMesh() const { return mesh; }

private:
  Mesh::SkeletalMeshPtr mesh;
};
using SkeletalMeshActorPtr = std::shared_ptr<SkeletalMeshActor>;

#endif // SKELETALMESHACTOR_H_INCLUDED