/**
* @file Collision.h
*/
#ifndef COLLISION_H_INCLUDED
#define COLLISION_H_INCLUDED
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <variant>

namespace Collision {

/**
* 球.
*/
struct Sphere {
  glm::vec3 center = glm::vec3(0);
  float r = 0;
};

/**
* 線分.
*/
struct Line {
  glm::vec3 a = glm::vec3(0);
  glm::vec3 b = glm::vec3(0);
};

/**
* カプセル.
*/
struct Capsule {
  Line line;
  float r = 0;
};

/**
* 有向境界ボックス
*/
struct OrientedBoundingBox {
  glm::vec3 center = glm::vec3(0);
  glm::vec3 axis[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
  glm::vec3 e = glm::vec3(0);
};

/**
* 汎用.
*/
using Shape = std::variant<Sphere, Capsule, OrientedBoundingBox>;

bool TestSphereSphere(const Sphere&, const Sphere&);
bool TestSphereCapsule(const Sphere& s, const Capsule& c, glm::vec3* p);
bool TestSphereOBB(const Sphere& s, const OrientedBoundingBox& obb, glm::vec3* p);

glm::vec3 ClosestPointLine(const Line& l, const glm::vec3& p);
glm::vec3 ClosestPointOBB(const OrientedBoundingBox& obb, const glm::vec3& p);




struct Ray {
  glm::vec3 position;
  glm::vec3 direction;
};

struct Plane {
  glm::vec3 normal = glm::vec3(0, 1, 0);
  float d = 0;
};

bool TestMovingSphereSphere(const Sphere& s0, const Sphere& s1, const glm::vec3& v0, const glm::vec3& v1, float* t);

} // namespace Collision

#endif // COLLISION_H_INCLUDED