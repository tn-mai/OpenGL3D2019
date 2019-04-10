/**
* @file Mesh.h
*/
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED
#include <GL/glew.h>
#include "BufferObject.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <string>

namespace Mesh {

// 頂点データ.
struct Vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
  glm::vec3 normal;
};

// 頂点属性.
struct VertexAttribute
{
  GLuint index = GL_INVALID_INDEX;
  GLint size = 0;
  GLenum type = GL_FLOAT;
  GLsizei stride = 0;
  size_t offset = 0;
};

/**
* 描画するメッシュデータ.
*
* Primitveをまとめたもの.
*/
struct Mesh
{
  std::string name;
  GLenum mode = GL_TRIANGLES;
  GLsizei count = 0;
  GLenum type = GL_UNSIGNED_SHORT;
  const GLvoid* indices = 0;
  GLint baseVertex = 0;
};

/**
* 全ての描画データを保持するクラス.
*
* メッシュの登録方法:
* - 頂点データとインデックスデータを用意する.
* - Meshオブジェクトを用意し、メッシュ名を決めてMesh::nameに設定する.
* - AddVertexData()で頂点の座標、テクスチャ座標、色などを設定し、戻り値をVertexサイズで割った値をbaseVertexに設定する.
* - AddIndexData()でインデックスデータを設定し、戻り値をMesh::indicesに設定する.
* - Meshのmode, count, typeメンバーに適切な値を設定する.
*   たとえば座標を頂点アトリビュートの0番目として指定する場合、Mesh::attributes[0]::offsetに設定する.
* - 頂点アトリビュートのindex, size, type, strideに適切な値を設定する.
* - AddMesh()でメッシュを登録する.
*/
class Buffer
{
public:
  Buffer() = default;
  ~Buffer() = default;

  bool Init(GLsizeiptr vboSize, GLsizeiptr iboSize);
  GLintptr AddVertexData(const void* data, size_t size);
  GLintptr AddIndexData(const void* data, size_t size);
  void AddMesh(const Mesh&);
  const Mesh& GetMesh(const char* meshName) const;
  void Bind();
  void Unbind();

  void CreateCircle(const char* name, size_t segments);

private:
  BufferObject vbo;
  BufferObject ibo;
  VertexArrayObject vao;
  GLintptr vboEnd = 0;
  GLintptr iboEnd = 0;
  std::unordered_map<std::string, Mesh> meshes;
};

} // namespace Mesh

#endif // MESH_H_INCLUDED