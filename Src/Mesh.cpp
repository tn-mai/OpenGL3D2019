/**
* @file Mesh.h
*/
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <math.h>
#include <iostream>

namespace Mesh {

/**
* メッシュを描画する.
*
* 事前にVAO、シェーダー、テクスチャ等をバインドしておくこと.
*/
void Mesh::Draw() const
{
  glDrawElementsBaseVertex(mode, count, type, indices, baseVertex);
}
  
/**
* メッシュバッファを初期化する.
*
* @param vboSize VBOのバイトサイズ.
* @param iboSize IBOのバイトサイズ.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool Buffer::Init(GLsizeiptr vboSize, GLsizeiptr iboSize)
{
  if (!vbo.Create(GL_ARRAY_BUFFER, vboSize)) {
    return false;
  }
  if (!ibo.Create(GL_ELEMENT_ARRAY_BUFFER, iboSize)) {
    return false;
  }
  if (!vao.Create(vbo.Id(), ibo.Id())) {
    return false;
  }
  vao.Bind();
  vao.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
  vao.VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, color));
  vao.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoord));
  vao.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
  vao.Unbind();
  vboEnd = 0;
  iboEnd = 0;
  meshes.reserve(100);
  return true;
}

/**
* 頂点データを追加する.
*
* @param data 追加するデータのポインタ.
* @param size 追加するデータのバイト数.
*
* @return データを追加した位置.
*         頂点アトリビュートのoffsetパラメータとして使うことができる.
*/
GLintptr Buffer::AddVertexData(const void* data, size_t size)
{
  vbo.BufferSubData(vboEnd, size, data);
  const GLintptr tmp = vboEnd;
  vboEnd += size;
  return tmp;
}

/**
* インデックスデータを追加する.
*
* @param data 追加するデータのポインタ.
* @param size 追加するデータのバイト数.
*
* @return データを追加した位置.
*         プリミティブのindicesパラメータとして使うことができる.
*/
GLintptr Buffer::AddIndexData(const void* data, size_t size)
{
  ibo.BufferSubData(iboEnd, size, data);
  const GLintptr tmp = iboEnd;
  iboEnd += size;
  return tmp;
}

/**
* メッシュを追加する.
*
* @param mesh 追加するメッシュ.
*/
void Buffer::AddMesh(const Mesh& mesh)
{
  meshes.emplace(mesh.name, mesh);
  std::cout << "Mesh::Buffer: メッシュ'" << mesh.name << "'を追加.\n";
}

/**
* メッシュを取得する.
*
* @param meshName 取得したいメッシュの名前.
*
* @return meshNameと同じ名前を持つメッシュ.
*/
const Mesh& Buffer::GetMesh(const char* meshName) const
{
  const auto itr = meshes.find(meshName);
  if (itr == meshes.end()) {
    static const Mesh dummy;
    return dummy;
  }
  return itr->second;
}

/**
* 立方体を作成する.
*
* @param 立方体のメッシュ名.
*/
void Buffer::CreateCube(const char* name)
{
  static const glm::vec3 basePositions[] = { {-1, 1, 1}, {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1} };
  static const glm::vec2 baseTexCoords[] = { { 0, 1}, { 0, 0}, { 1, 0}, { 1, 1} };
  static const GLubyte baseIndices[] = { 0, 1, 2, 2, 3, 0 };
  static const glm::vec3 normal(0, 0, 1);

  Vertex v;
  v.color = glm::vec4(1);
  std::vector<Vertex> vertices;
  std::vector<GLubyte> indices;
  for (int face = 0; face < 4; ++face) {
    const float angle = static_cast<float>(face) / 4.0f * glm::two_pi<float>();
    const glm::mat4 m = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0));
    v.normal = m * glm::vec4(normal, 1);
    for (size_t i = 0; i < 4; ++i) {
      v.position = m * glm::vec4(basePositions[i], 1);
      v.texCoord = baseTexCoords[i];
      vertices.push_back(v);
    }
    for (size_t i = 0; i < 6; ++i) {
      indices.push_back(static_cast<GLubyte>(baseIndices[i] + face * 4));
    }
  }
  for (int face = 0; face < 2; ++face) {
    const float angle = static_cast<float>(face) / 2.0f * glm::two_pi<float>() + glm::half_pi<float>();
    const glm::mat4 m = glm::rotate(glm::mat4(1), angle, glm::vec3(1, 0, 0));
    v.normal = m * glm::vec4(normal, 1);
    for (size_t i = 0; i < 4; ++i) {
      v.position = m * glm::vec4(basePositions[i], 1);
      v.texCoord = baseTexCoords[i];
      vertices.push_back(v);
    }
    for (size_t i = 0; i < 6; ++i) {
      indices.push_back(static_cast<GLubyte>(baseIndices[i] + (face + 4) * 4));
    }
  }
  const size_t vOffset = AddVertexData(vertices.data(), vertices.size() * sizeof(Vertex));
  const size_t iOffset = AddIndexData(indices.data(), indices.size() * sizeof(GLubyte));

  Mesh mesh;
  mesh.name = name;
  mesh.type = GL_UNSIGNED_BYTE;
  mesh.count = static_cast<GLsizei>(indices.size());
  mesh.indices = reinterpret_cast<const GLvoid*>(iOffset);
  mesh.baseVertex = vOffset / sizeof(Vertex);
  AddMesh(mesh);
}

/**
* 円盤を作成する.
*
* @param name     円盤のメッシュ名.
* @param segments 円盤の円周の分割数.
*/
void Buffer::CreateCircle(const char* name, size_t segments)
{
  if (segments < 3) {
    return;
  }

  std::vector<Vertex> vertices;
  vertices.reserve(segments + 1);

  Vertex center;
  center.position = glm::vec3(0);
  center.color = glm::vec4(1);
  center.texCoord = glm::vec2(0.5f);
  center.normal = glm::vec3(0, 1, 0);
  vertices.push_back(center);

  const float seg = static_cast<float>(segments);
  for (float i = 0; i < seg; ++i) {
    const glm::mat4 m = glm::rotate(glm::mat4(1), i / seg * glm::two_pi<float>(), glm::vec3(0, 1, 0));
    Vertex v;
    v.position = m * glm::vec4(1, 0, 0, 1);
    v.color = glm::vec4(1);
    v.texCoord = glm::vec2(v.position.x, v.position.z) * 0.5f + 0.5f;
    v.normal = glm::vec3(0, 1, 0);
    vertices.push_back(v);
  }
  const size_t vOffset = AddVertexData(vertices.data(), vertices.size() * sizeof(Vertex));

  std::vector<GLubyte> indices;
  indices.reserve(segments * 3);
  for (GLubyte i = 0; i < segments - 1; ++i) {
    indices.push_back(0);
    indices.push_back(i + 1);
    indices.push_back(i + 2);
  }
  indices.push_back(0);
  indices.push_back(static_cast<GLubyte>(segments));
  indices.push_back(1);
  const size_t iOffset = AddIndexData(indices.data(), indices.size() * sizeof(GLubyte));

  Mesh mesh;
  mesh.name = name;
  mesh.type = GL_UNSIGNED_BYTE;
  mesh.count = static_cast<GLsizei>(indices.size());
  mesh.indices = reinterpret_cast<const GLvoid*>(iOffset);
  mesh.baseVertex = vOffset / sizeof(Vertex);
  AddMesh(mesh);
}

/**
* 球を作成する.
*
* @param name     球のメッシュ名.
* @param segments 球の円周の分割数.
* @param rings    球の垂直方向の分割数.
*/
void Buffer::CreateSphere(const char* name, size_t segments, size_t rings)
{
  if (segments < 3 || rings < 2) {
    return;
  }

  std::vector<Vertex> vertices;
  vertices.reserve(segments * (rings - 1) + 2);

  // 一番上の頂点.
  Vertex center;
  center.position = glm::vec3(0, 1, 0);
  center.color = glm::vec4(1);
  center.texCoord = glm::vec2(0.5f, 1.0f);
  center.normal = glm::vec3(0, 1, 0);
  vertices.push_back(center);

  // 中間部の頂点.
  for (size_t ring = 1; ring < rings; ++ring) {
    const float ringAngle = static_cast<float>(ring) / static_cast<float>(rings) * glm::pi<float>();
    const glm::mat4 matRing = glm::rotate(glm::mat4(1), ringAngle, glm::vec3(1, 0, 0));
    const glm::vec4 axis = matRing * glm::vec4(0, 1, 0, 1);
    const float seg = static_cast<float>(segments);
    for (float i = 0; i < seg; ++i) {
      const glm::mat4 m = glm::rotate(glm::mat4(1), i / seg * glm::two_pi<float>(), glm::vec3(0, 1, 0));
      Vertex v;
      v.position = m * axis;
      v.color = glm::vec4(1);
      v.texCoord = glm::vec2(v.position.x, static_cast<float>(rings - i) / static_cast<float>(rings));
      v.normal = glm::normalize(v.position);
      vertices.push_back(v);
    }
  }
  // 一番下の頂点.
  center.position = glm::vec3(0, -1, 0);
  center.texCoord = glm::vec2(0.5f, 0.0f);
  center.normal = glm::vec3(0, -1, 0);
  vertices.push_back(center);
  const size_t vOffset = AddVertexData(vertices.data(), vertices.size() * sizeof(Vertex));

  std::vector<GLushort> indices;
  const size_t indexCountOfTopCone = segments * 3;
  const size_t indexCountOfCenterRing = segments * (rings - 2) * 3 * 2;
  const size_t indexCountOfBottomCone = segments * 3;
  indices.reserve(indexCountOfTopCone + indexCountOfCenterRing + indexCountOfBottomCone);

  // 上部の円錐.
  for (GLushort i = 0; i < segments - 1; ++i) {
    indices.push_back(0);
    indices.push_back(i + 1);
    indices.push_back(i + 2);
  }
  indices.push_back(0);
  indices.push_back(static_cast<GLushort>(segments));
  indices.push_back(1);

  // 中間部のリング.
  for (size_t ring = 1; ring < rings - 1; ++ring) {
    const GLushort upperBaseIndex = static_cast<GLushort>(segments * (ring - 1) + 1);
    const GLushort lowerBaseIndex = static_cast<GLushort>(segments * ring + 1);
    for (GLushort i = 0; i < segments - 1; ++i) {
      indices.push_back(i + upperBaseIndex);
      indices.push_back(i + lowerBaseIndex);
      indices.push_back(i + lowerBaseIndex + 1);
      indices.push_back(i + lowerBaseIndex + 1);
      indices.push_back(i + upperBaseIndex + 1);
      indices.push_back(i + upperBaseIndex);
    }
    const GLushort offset = static_cast<GLushort>(segments - 1);
    indices.push_back(offset + upperBaseIndex);
    indices.push_back(offset + lowerBaseIndex);
    indices.push_back(lowerBaseIndex);
    indices.push_back(lowerBaseIndex);
    indices.push_back(upperBaseIndex);
    indices.push_back(offset + upperBaseIndex);
  }

  // 下部の円錐.
  const GLushort bottomIndex = static_cast<GLushort>(segments * (rings - 1) + 1);
  const GLushort baseIndex = static_cast<GLushort>(segments * (rings - 2) + 1);
  for (GLushort i = 0; i < segments - 1; ++i) {
    indices.push_back(bottomIndex);
    indices.push_back(i + baseIndex + 1);
    indices.push_back(i + baseIndex);
  }
  indices.push_back(bottomIndex);
  indices.push_back(baseIndex);
  indices.push_back(bottomIndex - 1);

  const size_t iOffset = AddIndexData(indices.data(), indices.size() * sizeof(GLushort));

  Mesh mesh;
  mesh.name = name;
  mesh.type = GL_UNSIGNED_SHORT;
  mesh.count = static_cast<GLsizei>(indices.size());
  mesh.indices = reinterpret_cast<const GLvoid*>(iOffset);
  mesh.baseVertex = vOffset / sizeof(Vertex);
  AddMesh(mesh);
}

/**
* メッシュ用VAOをバインドする.
*/
void Buffer::Bind()
{
  vao.Bind();
}

/**
* メッシュ用VAOのバインドを解除する.
*/
void Buffer::Unbind()
{
  vao.Unbind();
}

} // namespace Mesh