/**
* @file Collision.cpp
*/
#include "Collision.h"

/**
* �Փ˔�����i�[���閼�O���.
*
* ���A�J�v�Z���AOBB������.
*
* - �� vs ��
* - �� vs �J�v�Z��
* - �� vs OBB
*/
namespace Collision {

/**
* ���Ƌ����Փ˂��Ă��邩���ׂ�.
*/
bool TestSphereSphere(const Sphere& s0, const Sphere& s1)
{
  const glm::vec3 m = s0.center - s1.center;
  const float radiusSum = s0.r + s1.r;
  return dot(m, m) <= (radiusSum * radiusSum);
  return true;
}

/**
* �����Ɠ_�̍ŋߐړ_�𒲂ׂ�.
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
* ���ƃJ�v�Z�����Փ˂��Ă��邩���ׂ�.
*/
bool TestSphereCapsule(const Sphere& s, const Capsule& c, glm::vec3* p)
{
  *p = ClosestPointLine(c.seg, s.center);
  const glm::vec3 distance = *p - s.center;
  const float radiusSum = s.r + c.r;
  return dot(distance, distance) <= radiusSum * radiusSum;
}

/**
* OBB�Ɠ_�̍ŋߐړ_�𒲂ׂ�.
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
* ����OBB���Փ˂��Ă��邩���ׂ�.
*/
bool TestSphereOBB(const Sphere& s, const OrientedBoundingBox& obb, glm::vec3* p)
{
  *p = ClosestPointOBB(obb, s.center);
  const glm::vec3 distance = *p - s.center;
  return dot(distance, distance) <= s.r * s.r;
}

/**
* ���`����쐬����.
*
* @param center ���̒��S���W.
* @param r      ���̔��a.
*
* @return ����ێ�����ėp�Փˌ`��I�u�W�F�N�g.
*/
Shape CreateSphere(const glm::vec3& center, float r) {
  Shape result;
  result.type = Shape::Type::sphere;
  result.s = Sphere{ center, r };
  return result;
}

/**
* �J�v�Z���`����쐬����.
*
* @param a  ���S�̐����̎n�_���W.
* @param b  ���S�̐����̏I�_���W.
* @param r  �J�v�Z���̔��a.
*
* @return �J�v�Z����ێ�����ėp�Փˌ`��I�u�W�F�N�g.
*/
Shape CreateCapsule(const glm::vec3& a, const glm::vec3& b, float r) {
  Shape result;
  result.type = Shape::Type::capsule;
  result.c = Capsule{ { a, b }, r };
  return result;
}

/**
* �L�����E�{�b�N�X�`����쐬����.
*
* @param center �L�����E�{�b�N�X�̒��S���W.
* @param axisX  X���̌���.
* @param axisY  Y���̌���.
* @param axisZ  Z���̌���.
* @param e      XYZ�������̕�.
*
* @return �L�����E�{�b�N�X��ێ�����ėp�Փˌ`��I�u�W�F�N�g.
*/
Shape CreateOBB(const glm::vec3& center, const glm::vec3& axisX, const glm::vec3& axisY, const glm::vec3& axisZ, const glm::vec3& e) {
  Shape result;
  result.type = Shape::Type::obb;
  result.obb = OrientedBoundingBox{ center, { axisX, axisY, axisZ }, e };
  return result;
}




/**
* �ړ����鋅�Ƌ����Փ˂��Ă��邩���ׂ�.
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

  // (c > 0)�Ȃ��r�̌��_��s�̊O���ɂ���. (b > 0)�Ȃ��r��s���痣��Ă����������w���Ă���.
  // ��L�̗����̏��������������ꍇ�A�����͔������Ȃ�.
  if (c > 0 && b > 0) {
    return false;
  }

  // ���ʎ������̏ꍇ�͉��Ȃ�(���������Ȃ�)
  const float d = b * b - c;
  if (d < 0) {
    return false;
  }

  // ����������
  *t = -b - std::sqrt(d);
  if (*t < 0) {
    *t = 0;
  }
  *p = r.position + r.direction * *t;
  return true;
}

} // namespace Collision