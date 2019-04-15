/**
* @file Terrain.cpp
*/
#include "Terrain.h"
#include "Texture.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace Terrain {

/**
* 画像ファイルから地形データを読み込む.
*/
bool HeightMap::Load(const char* path, float scale, float middleLevel)
{
  Texture::ImageData imageData;
  if (!Texture::LoadImage2D(path, imageData)) {
    return false;
  }
  if (imageData.format != GL_RED) {
    return false;
  }
  size = glm::ivec2(imageData.width, imageData.height);
  heights.reserve(imageData.data.size());
  for (auto e : imageData.data) {
    heights.push_back((e / 255.0f - middleLevel) * scale);
  }
  return true;
}

glm::vec3 HeightMap::CalcNormal(int centerX, int centerZ) const
{
  const int minX = std::max(centerX - 1, 0);
  const int maxX = std::min(centerX + 1, size.x - 1);
  const int minZ = std::max(centerZ - 1, 0);
  const int maxZ = std::min(centerZ + 1, size.y - 1);
  const glm::vec3 center(centerX, heights[centerZ * size.y + centerX], size.y - centerZ);
  glm::vec3 normal(0);
  for (int z = minZ; z <= maxZ; ++z) {
	  if (z == centerZ) {
		  continue;
	  }
    for (int x = minX; x <= maxX; ++x) {
      if (x == centerX) {
        continue;
      }
      const glm::vec3 p(x, heights[z * size.y + x], size.y - z);
      const glm::vec3 v0 = normalize(p - center);
      const glm::vec3 tangent = normalize(cross(glm::vec3(0, 1, 0), v0));
      normal += normalize(cross(v0, tangent));
    }
  }
  return normalize(normal);
}


/**
* 高さマップからメッシュを作成する.
*/
bool HeightMap::CreateMesh(Mesh::Buffer& meshBuffer, const char* name)
{
  if (heights.empty()) {
    return false;
  }
  Mesh::Vertex v;
  v.color = glm::vec4(1);
  std::vector<Mesh::Vertex> vertices;
  vertices.reserve(size.x * size.y);
  for (int z = 0; z < size.y; ++z) {
    for (int x = 0; x < size.x; ++x) {
      v.position = glm::vec3(x, heights[z * size.x + x], size.y - z);
      v.texCoord = glm::vec2(x, z) / (glm::vec2(size) - 1.0f);
      v.normal = CalcNormal(x, z);
      vertices.push_back(v);
    }
  }
  const size_t vOffset = meshBuffer.AddVertexData(vertices.data(), vertices.size() * sizeof(Mesh::Vertex));

  std::vector<GLushort> indices;
  indices.reserve(size.x * size.y);
  for (int z = 0; z < size.y - 1; ++z) {
    for (int x = 0; x < size.x - 1; ++x) {
      indices.push_back(z * size.x + x);
      indices.push_back((z + 1) * size.x + x + 1);
      indices.push_back((z + 1) * size.x + x);
      indices.push_back((z + 1) * size.x + x + 1);
      indices.push_back(z * size.x + x);
      indices.push_back(z * size.x + x + 1);
    }
  }
  const size_t iOffset = meshBuffer.AddIndexData(indices.data(), indices.size() * sizeof(GLushort));

  Mesh::Mesh mesh;
  mesh.name = "Terrain";
  mesh.baseVertex = vOffset / sizeof(Mesh::Vertex);
  mesh.count = indices.size();
  mesh.indices = reinterpret_cast<const GLvoid*>(iOffset);
  mesh.mode = GL_TRIANGLES;
  mesh.type = GL_UNSIGNED_SHORT;
  meshBuffer.AddMesh(mesh);

  return true;
}

/**
* 高さを取得する.
*/
float HeightMap::Height(const glm::vec3& pos) const
{
  return 0;
}

} // namespace Terrain