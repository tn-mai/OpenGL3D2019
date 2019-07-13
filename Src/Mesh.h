/**
* @file Mesh.h
*/
#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED
#include <GL/glew.h>
#include "BufferObject.h"
#include "json11/json11.hpp"
#include "Texture.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace glm {
using aligned_quat = qua<float, aligned_highp>;
}

namespace Mesh {

// 先行宣言.
class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;
struct Mesh;
using MeshPtr = std::shared_ptr<Mesh>;
class SkeletalMesh;
using SkeletalMeshPtr = std::shared_ptr<SkeletalMesh>;

// スケルタルメッシュ用.
struct Node;
struct ExtendedFile;
using ExtendedFilePtr = std::shared_ptr<ExtendedFile>;

// 頂点データ.
struct Vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
  glm::vec3 normal;
  GLfloat   weights[4];
  GLushort  joints[4];
};

// マテリアル.
struct Material {
  glm::vec4 baseColor = glm::vec4(1);
  Texture::Image2DPtr texture;
  Shader::ProgramPtr program;
  Shader::ProgramPtr progSkeletalMesh;
};

// メッシュプリミティブ.
struct Primitive {
  GLenum mode;
  GLsizei count;
  GLenum type;
  const GLvoid* indices;
  GLint baseVertex = 0;
  std::shared_ptr<VertexArrayObject> vao;
  bool hasColorAttribute = false;
  size_t material = 0;
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
};
using FilePtr = std::shared_ptr<File>;

/**
* 描画するメッシュデータ.
*
* Primitveをまとめたもの.
* シーンルートノードからメッシュノードまでの変換行列を掛けないと、正しい姿勢で描画されない場合がある.
* アニメーションを持つのがボーンノードだけなら、これは一度計算すれば十分.
* だが、通常ノードがアニメーションを持っている場合もあるので、その場合は毎回ルートノードから再計算する必要がある.
* glTFのアニメーションデータには通常ノードとボーンノードの区別がないので、区別したければアニメーションデータの対象となるノードが
* スケルトンノードリストに含まれるかどうかで判定する必要がありそう.
*
* 作成方法:
* - Buffer::GetMeshで作成.
*
* 更新方法:
* -# Buffer::ResetUniformData()でUBOバッファをリセット.
* -# 描画するすべてのメッシュについて、Mesh::UpdateでアニメーションとUBOバッファの更新(これは分けたほうがいいかも).
* -# Buffer::UploadUniformData()でUBOバッファをVRAMに転送.
*
* 描画方法:
* - Mesh::Drawで描画.
*/
struct Mesh
{
  Mesh() = default;
  Mesh(const FilePtr& f, int n) : file(f), meshNo(n) {}
  void Draw(const glm::mat4& matModel) const;

  std::string name;
  FilePtr file;
  int meshNo = -1;
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

  bool Init(GLsizeiptr vboSize, GLsizeiptr iboSize, GLsizeiptr uboSize);
  GLintptr AddVertexData(const void* data, size_t size);
  GLintptr AddIndexData(const void* data, size_t size);
  Primitive CreatePrimitive(size_t count, GLenum type, size_t iOffset, size_t vOffset) const;
  Material CreateMaterial(const glm::vec4& color, Texture::Image2DPtr texture) const;
  void AddMesh(const char* name, size_t count, GLenum type, size_t iOffset, size_t vOffset);
  bool AddMesh(const char* name, const Primitive& primitive, const Material& material);
  bool LoadMesh(const char* path);
  MeshPtr GetMesh(const char* meshName) const;
  void Bind();
  void Unbind();

  void SetViewProjectionMatrix(const glm::mat4&) const;

  void CreateCube(const char* name);
  void CreateCircle(const char* name, size_t segments);
  void CreateSphere(const char* name, size_t segments, size_t rings);

  // スケルタルメッシュ用.
  bool LoadSkeletalMesh(const char* path);
  SkeletalMeshPtr GetSkeletalMesh(const char* meshName) const;

private:
  BufferObject vbo;
  BufferObject ibo;
  VertexArrayObject vao;
  GLintptr vboEnd = 0;
  GLintptr iboEnd = 0;

  std::unordered_map<std::string, FilePtr> files;
  Shader::ProgramPtr progStaticMesh;

  bool SetAttribute(
    Primitive& prim, int index, const json11::Json& accessor, const json11::Json& bufferViews, const std::vector<std::vector<char>>& binFiles);

  // スケルタルメッシュ用.
  struct MeshIndex {
    ExtendedFilePtr file;
    const Node* node = nullptr;
  };
  std::unordered_map<std::string, MeshIndex> meshes;
  std::unordered_map<std::string, ExtendedFilePtr> extendedFiles;
  Shader::ProgramPtr progSkeletalMesh;
};

} // namespace Mesh

#endif // MESH_H_INCLUDED