/**
* @file Collision.cpp
*/
#include "Collision.h"

/**
* 衝突判定を格納する名前空間.
*
* 球、カプセル、OBBを扱う.
*
* - 球 vs 球
* - 球 vs カプセル
* - 球 vs OBB
*/
namespace Collision {

/**
* 球と球が衝突しているか調べる.
*/
bool TestSphereSphere(const Sphere& s0, const Sphere& s1)
{
  const glm::vec3 m = s0.center - s1.center;
  const float radiusSum = s0.r + s1.r;
  return dot(m, m) <= (radiusSum * radiusSum);
  return true;
}

/**
* 線分と点の最近接点を調べる.
*/
glm::vec3 ClosestPointLine(const Segment& l, const glm::vec3& p)
{
  const glm::vec3 v = l.b - l.a;
  const glm::vec3 m = p - l.a;
  const float vm = dot(v, m);
  const float vv = dot(v, v);
  if (vm < 0) {
    return l.a;
  } else if (vm > vv) {
    return l.b;
  }
  return l.a + v * vm / vv;
}

/**
* 球とカプセルが衝突しているか調べる.
*/
bool TestSphereCapsule(const Sphere& s, const Capsule& c, glm::vec3* p)
{
  *p = ClosestPointLine(c.seg, s.center);
  const glm::vec3 distance = *p - s.center;
  const float radiusSum = s.r + c.r;
  return dot(distance, distance) <= radiusSum * radiusSum;
}

/**
* OBBと点の最近接点を調べる.
*/
glm::vec3 ClosestPointOBB(const OrientedBoundingBox& obb, const glm::vec3& p)
{
  const glm::vec3 d = p - obb.center;
  glm::vec3 q = obb.center;
  for (int i = 0; i < 3; ++i) {
    float distance = dot(d, obb.axis[i]);
    if (distance > obb.e[i]) {
      distance = obb.e[i];
    } else if (distance < -obb.e[i]) {
      distance = -obb.e[i];
    }
    q += distance * obb.axis[i];
  }
  return q;
}

/**
* 球とOBBが衝突しているか調べる.
*/
bool TestSphereOBB(const Sphere& s, const OrientedBoundingBox& obb, glm::vec3* p)
{
  *p = ClosestPointOBB(obb, s.center);
  const glm::vec3 distance = *p - s.center;
  return dot(distance, distance) <= s.r * s.r;
}

/**
* 球形状を作成する.
*
* @param center 球の中心座標.
* @param r      球の半径.
*
* @return 球を保持する汎用衝突形状オブジェクト.
*/
Shape CreateSphere(const glm::vec3& center, float r) {
  Shape result;
  result.type = Shape::Type::sphere;
  result.s = Sphere{ center, r };
  return result;
}

/**
* カプセル形状を作成する.
*
* @param a  中心の線分の始点座標.
* @param b  中心の線分の終点座標.
* @param r  カプセルの半径.
*
* @return カプセルを保持する汎用衝突形状オブジェクト.
*/
Shape CreateCapsule(const glm::vec3& a, const glm::vec3& b, float r) {
  Shape result;
  result.type = Shape::Type::capsule;
  result.c = Capsule{ { a, b }, r };
  return result;
}

/**
* 有向境界ボックス形状を作成する.
*
* @param center 有向境界ボックスの中心座標.
* @param axisX  X軸の向き.
* @param axisY  Y軸の向き.
* @param axisZ  Z軸の向き.
* @param e      XYZ軸方向の幅.
*
* @return 有向境界ボックスを保持する汎用衝突形状オブジェクト.
*/
Shape CreateOBB(const glm::vec3& center, const glm::vec3& axisX, const glm::vec3& axisY, const glm::vec3& axisZ, const glm::vec3& e) {
  Shape result;
  result.type = Shape::Type::obb;
  result.obb = OrientedBoundingBox{ center, { axisX, axisY, axisZ }, e };
  return result;
}




/**
* 移動する球と球が衝突しているか調べる.
*/
bool TestMovingSphereSphere(const Sphere& s0, const Sphere& s1, const glm::vec3& v0, const glm::vec3& v1, float* t)
{
  const glm::vec3 s = s1.center - s0.center;
  const float r = s1.r + s0.r;
  const float c = dot(s, s) - r * r;
  if (c < 0) {
    *t = 0;
    return true;
  }

  const glm::vec3 v = v1 - v0;
  const float a = dot(v, v);
  if (a < FLT_EPSILON) {
    return false;
  }

  const float b = dot(v, s);
  if (b >= 0) {
    return false;
  }

  const float d = b * b - a * c;
  if (d < 0) {
    return false;
  }
  *t = (-b - std::sqrt(d)) / a;
  return true;
}

/**
*
*/
bool FindIntersectionRaySphere(const Ray& r, const Sphere& s, float* t, glm::vec3* p)
{
  // m.m + m.td + m.td + td.td = r^2
  // m.m + 2(m.d)t + (d.d)t^2 = r^2
  // m.m + 2(m.d)t + t^2 = r^2
  // t^2 + 2(m.d)t + m.m - r^2 = 0
  // t^2 + 2bt + c = 0 -> b = m.d, c = m.m - r^2
  // t = -b +- sqrt(b^2 - c)
  const glm::vec3 m = r.position - s.center;
  const float b = dot(m, r.direction);
  const float c = dot(m, m) - s.r * s.r;

  // (c > 0)ならばrの原点はsの外側にある. (b > 0)ならばrはsから離れていく方向を指している.
  // 上記の両方の条件が満たされる場合、交差は発生しない.
  if (c > 0 && b > 0) {
    return false;
  }

  // 判別式が負の場合は解なし(＝交差しない)
  const float d = b * b - c;
  if (d < 0) {
    return false;
  }

  // 小さい方の
  *t = -b - std::sqrt(d);
  if (*t < 0) {
    *t = 0;
  }
  *p = r.position + r.direction * *t;
  return true;
}

} // namespace Collision