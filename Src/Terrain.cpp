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
* �摜�t�@�C������n�`�f�[�^��ǂݍ���.
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
  // �摜�͉������Ɍ������Ċi�[����Ă���̂ŁA�㉺���]���Ȃ��獂���f�[�^�ɕϊ�.
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
* �����}�b�v���烁�b�V�����쐬����.
*
* �O�p�`�͈ȉ��̌`�ɂȂ�.
*   +--+
*   |�^|
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
      v.texCoord = glm::vec2(x, (size.y - 1) - z) / (glm::vec2(size) - 1.0f); // �e�N�X�`�����W�͏オ�v���X�Ȃ̂Ō������t�ɂ���K�v������.
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

  meshBuffer.AddMesh("Terrain", indices.size(), GL_UNSIGNED_SHORT, iOffset, vOffset);

  return true;
}

/**
* �������擾����.
*/
float HeightMap::Height(const glm::vec3& pos) const
{
  // ���W�������}�b�v�͈͓̔��Ɏ��܂�悤�ɐ؂�グ�A�܂��͐؂�̂Ă�.
  const glm::vec2 fpos = glm::clamp(glm::vec2(pos.x, pos.z), glm::vec2(0.0f), glm::vec2(size) - glm::vec2(1));

  // �E�[�Ɖ��[��1��͍�����W�Ƃ��Ďg���Ȃ��̂�size-2�ɐ���.
  // ���Wpos��(x,y), (x+1,y), (x,Y+1), (x+1,y+1)��4�_�̂Ȃ������`�̓����ɑ��݂���.
  const glm::ivec2 index = glm::min(glm::ivec2(fpos), size - glm::ivec2(2));

  // �C���f�b�N�X���W����̑��΍��W���v�Z.
  const glm::vec2 offset = fpos - glm::vec2(index);

  // �n�`��4���_����Ȃ鐳���`�ł���A�����`�͉��L�̂悤��2�̎O�p�`�Ƃ��Ē�`�����.
  //     -Y
  //    +--+
  // -X |�^| +X
  //    +--+
  //     +Y
  // ����-Y�����A��O��+Y�����ŁA1x1�̐����`�ł��邱�Ƃ���A
  // ���΍��W���uX + Y < 0�v�Ȃ�΁A���΍��W�͍���̎O�p�`�̈�ɑ��݂���.
  // �����łȂ���΁A���΍��W�͉E���̎O�p�`�̈�ɑ��݂���.
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