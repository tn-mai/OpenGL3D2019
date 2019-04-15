/**
* @file Terrain.h
*/
#ifndef TERRAIN_H_INCLUDED
#define TERRAIN_H_INCLUDED
#include "Mesh.h"
#include <glm/glm.hpp>
#include <vector>

namespace Terrain {

/**
* çÇÇ≥É}ÉbÉv.
*/
class HeightMap {
public:
  HeightMap() = default;
  ~HeightMap() = default;

  bool Load(const char* path, float scale, float middleLevel);
  bool CreateMesh(Mesh::Buffer& meshBuffer, const char* name);
  float Height(const glm::vec3& pos) const;
  const glm::ivec2& Size() const { return size; }

private:
  glm::ivec2 size = glm::ivec2(0);
  std::vector<float> heights;

  glm::vec3 CalcNormal(int x, int z) const;
};

} // namespace Terrain

#endif // TERRAIN_H_INCLUDED