/**
* @file Terrain.h
*/
#ifndef TERRAIN_H_INCLUDED
#define TERRAIN_H_INCLUDED
#include "Mesh.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Terrain {

/**
* 高さマップ.
*/
class HeightMap {
public:
  HeightMap() = default;
  ~HeightMap() = default;

  bool Load(const char* path, float scale, float middleLevel);
  bool CreateMesh(Mesh::Buffer& meshBuffer, const char* name) const;
  float Height(const glm::vec3& pos) const;
  const glm::ivec2& Size() const { return size; }
  const std::string& Name() const { return name; }

private:
  std::string name; ///< 元になった画像ファイル名.
  glm::ivec2 size = glm::ivec2(0); ///< ハイトマップの大きさ.
  std::vector<float> heights; ///< 高さデータ.

  glm::vec3 CalcNormal(int x, int z) const;
};

} // namespace Terrain

#endif // TERRAIN_H_INCLUDED