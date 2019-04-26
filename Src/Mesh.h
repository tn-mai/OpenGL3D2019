/**
* @file Mesh.h
*/
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED
#include <GL/glew.h>
#include "BufferObject.h"
#include "json11/json11.hpp"
#include "Texture.h"
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

// マテリアル.
struct Material {
  glm::vec4 baseColor = glm::vec4(1);
  Texture::Image2DPtr texture;
};

// メッシュプリミティブ.
struct Primitive {
  GLenum mode;
  GLsizei count;
  GLenum type;
  const GLvoid* indices;
  GLint baseVertex = 0;
  VertexAttribute attributes[8];
  int material;
};

// メッシュデータ.
struct MeshData {
  std::string name;
  std::vector<Primitive> primitives;
};

// ファイル.
struct File {
  std::string name; // ファイル名.
  std::vector<MeshData> meshes;
  std::vector<Material> materials;
  const VertexArrayObject* vao = nullptr;
};
using FilePtr = std::shared_ptr<File>;

/**
* 描画するメッシュデータ.
*
* Primitveをまとめたもの.
*/
struct Mesh
{
  Mesh() = default;
  Mesh(const FilePtr& f, int id) : file(f), meshId(id) {}
  void Draw() const;

  std::string name;

  GLenum mode = GL_TRIANGLES;
  GLsizei count = 0;
  GLenum type = GL_UNSIGNED_SHORT;
  const GLvoid* indices = 0;
  GLint baseVertex = 0;

  int meshId = 0;
  FilePtr file;
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
  bool LoadMesh(const char* path);
  Mesh GetMesh(const char* meshName) const;
  void Bind();
  void Unbind();

  void CreateCube(const char* name);
  void CreateCircle(const char* name, size_t segments);
  void CreateSphere(const char* name, size_t segments, size_t rings);

private:
  BufferObject vbo;
  BufferObject ibo;
  VertexArrayObject vao;
  GLintptr vboEnd = 0;
  GLintptr iboEnd = 0;
  struct MeshIndex {
    FilePtr file;
    size_t index = 0;
  };
  std::unordered_map<std::string, MeshIndex> meshes;
  std::unordered_map<std::string, FilePtr> files;

  bool SetAttribute(
    Primitive& prim, const json11::Json& accessor, const json11::Json& bufferViews, std::vector<std::vector<char>>& binFiles, int index, int size);
};

} // namespace Mesh

#endif // MESH_H_INCLUDED