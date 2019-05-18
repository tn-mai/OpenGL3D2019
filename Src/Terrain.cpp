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
  // 画像は下から上に向かって格納されているので、上下反転しながら高さデータに変換.
  heights.resize(imageData.data.size());
  for (int y = 0; y < size.y; ++y) {
    for (int x = 0; x < size.x; ++x) {
      const uint8_t color = imageData.data[y * size.x + x];
      heights[(size.y - y - 1) * size.x + x] = (color / 255.0f - middleLevel) * scale;
    }
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
*
* 三角形は以下の形になる.
*   +--+
*   |／|
*   +--+
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
      v.position = glm::vec3(x, heights[z * size.x + x], z);
      v.texCoord = glm::vec2(x, (size.y - 1) - z) / (glm::vec2(size) - 1.0f); // テクスチャ座標は上がプラスなので向きを逆にする必要がある.
      v.normal = CalcNormal(x, z);
      vertices.push_back(v);
    }
  }
  const size_t vOffset = meshBuffer.AddVertexData(vertices.data(), vertices.size() * sizeof(Mesh::Vertex));

  std::vector<GLushort> indices;
  indices.reserve(size.x * size.y);
  for (int z = 0; z < size.y - 1; ++z) {
    for (int x = 0; x < size.x - 1; ++x) {
      indices.push_back(z * size.x       + x);
      indices.push_back((z + 1) * size.x + x);
      indices.push_back(z * size.x       + (x + 1));

      indices.push_back((z + 1) * size.x + (x + 1));
      indices.push_back(z * size.x + (x + 1));
      indices.push_back((z + 1) * size.x + x);
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
  // 座標が高さマップの範囲内に収まるように切り上げ、または切り捨てる.
  const glm::vec2 fpos = glm::clamp(glm::vec2(pos.x, pos.z), glm::vec2(0.0f), glm::vec2(size) - glm::vec2(1));

  // 右端と下端の1列は左上座標として使えないのでsize-2に制限.
  // 座標posは(x,y), (x+1,y), (x,Y+1), (x+1,y+1)の4点のなす正方形の内側に存在する.
  const glm::ivec2 index = glm::min(glm::ivec2(fpos), size - glm::ivec2(2));

  // インデックス座標からの相対座標を計算.
  const glm::vec2 offset = fpos - glm::vec2(index);

  // 地形は4頂点からなる正方形であり、正方形は下記のように2つの三角形として定義される.
  //     -Y
  //    +--+
  // -X |／| +X
  //    +--+
  //     +Y
  // 奥が-Y方向、手前が+Y方向で、1x1の正方形であることから、
  // 相対座標が「X + Y < 0」ならば、相対座標は左上の三角形領域に存在する.
  // そうでなければ、相対座標は右下の三角形領域に存在する.
  if (offset.x + offset.y < 1) {
    const float h0 = heights[index.y * size.x       + index.x];
    const float h1 = heights[index.y * size.x       + (index.x + 1)];
    const float h2 = heights[(index.y + 1) * size.x + index.x];
    return h0 + (h1 - h0) * offset.x + (h2 - h0) * offset.y;
  } else {
    const float h0 = heights[(index.y + 1) * size.x + (index.x + 1)];
    const float h1 = heights[(index.y + 1) * size.x + index.x];
    const float h2 = heights[index.y * size.x       + (index.x + 1)];
    return h0 + (h1 - h0) * (1.0f - offset.x) + (h2 - h0) * (1.0f - offset.y);
  }
}

} // namespace Terrain