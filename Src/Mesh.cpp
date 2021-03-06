/**
* @file Mesh.h
*/
#define NOMINMAX
#include "Mesh.h"
#include "SkeletalMesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Mesh {

/**
* ファイルを読み込む.
*
* @param path ファイル名.
*
* @return ファイルの内容.
*/
std::vector<char> ReadFromFile(const char* path)
{
  std::ifstream ifs(path, std::ios_base::binary);
  if (!ifs) {
    std::cerr << "ERROR: " << path << "を開けません.\n";
    return {};
  }
  std::vector<char> tmp(1000000);
  ifs.rdbuf()->pubsetbuf(tmp.data(), tmp.size());

  ifs.seekg(0, std::ios_base::end);
  const std::streamoff size = ifs.tellg();
  ifs.seekg(0, std::ios_base::beg);

  std::vector<char> buf;
  buf.resize(static_cast<size_t>(size));
  ifs.read(&buf[0], size);

  return buf;
}

/**
* プリミティブを作成する.
*
* @param count    プリミティブのインデックスデータの数.
* @param type     インデックスデータの型(GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INTのいずれか).
* @param iOffset  IBO内のインデックスデータの開始位置.
* @param vOffset  VBO内の頂点データの開始位置.
*
* @return 作成したPrimitive構造体.
*/
Primitive Buffer::CreatePrimitive(size_t count, GLenum type, size_t iOffset, size_t vOffset) const
{
  Primitive prim;
  prim.mode = GL_TRIANGLES;
  prim.count = static_cast<GLsizei>(count);
  prim.type = type;
  prim.indices = reinterpret_cast<const GLvoid*>(iOffset);
  prim.baseVertex = 0;
  std::shared_ptr<VertexArrayObject> vao = std::make_shared<VertexArrayObject>();
  vao->Create(vbo.Id(), ibo.Id());
  vao->Bind();
  vao->VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), vOffset + offsetof(Vertex, position));
  vao->VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), vOffset + offsetof(Vertex, color));
  vao->VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), vOffset + offsetof(Vertex, texCoord));
  vao->VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), vOffset + offsetof(Vertex, normal));
  vao->Unbind();
  prim.vao = vao;
  prim.hasColorAttribute = true;
  prim.material = 0;

  return prim;
}

/**
* メッシュを描画する.
*
* 事前にVAOをバインドしておくこと.
*/
void Mesh::Draw(const glm::mat4& matModel, const glm::vec4& color) const
{
  if (!file || meshNo < 0) {
    return;
  }

  // TODO: シーンレベルの描画に対応すること.
  //std::vector<const Node*> meshNodes;
  //meshNodes.reserve(32);
  //GetMeshNodeList(node, meshNodes);

  const MeshData& meshData = file->meshes[meshNo];
  GLuint prevTexId = 0;
  for (const auto& prim : meshData.primitives) {
    prim.vao->Bind();
    if (!prim.hasColorAttribute) {
      static const glm::vec4 color(1);
      glVertexAttrib4fv(1, &color.x);
    }

    if (prim.material >= 0 && prim.material < static_cast<int>(file->materials.size())) {
      const Material& m = file->materials[prim.material];
      m.program->Use();
      m.program->SetModelMatrix(matModel);

      if (m.texture) {
        const GLuint texId = m.texture->Id();
        if (prevTexId != texId) {
          m.texture->Bind(0);
          prevTexId = texId;
        }
      }
      m.program->SetModelColor(color);
    }
    glDrawElementsBaseVertex(prim.mode, prim.count, prim.type, prim.indices, prim.baseVertex);
    prim.vao->Unbind();
  }
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
bool Buffer::Init(GLsizeiptr vboSize, GLsizeiptr iboSize, GLsizeiptr uboSize)
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

  Shader::Cache& shaderCache = Shader::Cache::Instance();
  progStaticMesh = shaderCache.Create("Res/Mesh.vert", "Res/Mesh.frag");
  progSkeletalMesh = shaderCache.Create("Res/SkeletalMesh.vert", "Res/SkeletalMesh.frag");

  GlobalSkeletalMeshState::BindUniformBlock(progSkeletalMesh);

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
* マテリアルを作成する.
*
* @param color   マテリアルの基本色.
* @param texture マテリアルのテクスチャ.
*
* @return 作成したMaterial構造体.
*/
Material Buffer::CreateMaterial(const glm::vec4& color, Texture::Image2DPtr texture) const
{
  Material m;
  m.baseColor = color;
  m.texture = texture;
  m.program = progStaticMesh;
  m.progSkeletalMesh = progSkeletalMesh;
  return m;
}

/**
* メッシュを追加する.
*
* @param data 追加するメッシュデータ.
*/
void Buffer::AddMesh(const char* name, size_t count, GLenum type, size_t iOffset, size_t vOffset)
{
  FilePtr pFile = std::make_shared<File>();
  pFile->name = name;

  MeshData data;
  data.name = name;
  data.primitives.push_back(CreatePrimitive(count, type, iOffset, vOffset));
  pFile->meshes.push_back(data);

  pFile->materials.push_back(CreateMaterial(glm::vec4(1), nullptr));
  if (files.find(name) != files.end()) {
    std::cerr << "[警告]" << __func__ << ": " << pFile->name << "という名前は登録済みです.\n";
  }
  files.insert(std::make_pair(pFile->name, pFile));
  std::cout << "Mesh::Buffer: メッシュ'" << name << "'を追加.\n";
}

/**
* メッシュを追加する.
*
* @param name      メッシュ及びファイルの名前.
* @param primitive メッシュとして追加するプリミティブ.
* @param material  プリミティブ用のマテリアル.
*
* @retval true  追加成功.
* @retval false 追加失敗(同名のメッシュが登録済み).
*/
bool Buffer::AddMesh(const char* name, const Primitive& primitive, const Material& material)
{
  if (files.find(name) != files.end()) {
    std::cerr << "[警告]" << __func__ << ": " << name <<
      "という名前は既に追加されています.\n";
    return false;
  }

  FilePtr p = std::make_shared<File>();
  p->name = name;
  p->materials.push_back(material);
  p->meshes.resize(1);
  p->meshes[0].name = name;
  p->meshes[0].primitives.push_back(primitive);

  files.insert(std::make_pair(p->name, p));
  std::cout << "[情報]" << __func__ << ": メッシュ'" << name << "'を追加.\n";
  return true;
}

/**
* メッシュを取得する.
*
* @param meshName 取得したいメッシュの名前.
*
* @return meshNameと同じ名前を持つメッシュ.
*/
MeshPtr Buffer::GetMesh(const char* meshName) const
{
  for (const auto& f : files) {
    for (size_t i = 0; i < f.second->meshes.size(); ++i) {
      if (f.second->meshes[i].name == meshName) {
        return std::make_shared<Mesh>(f.second, static_cast<int>(i));
      }
    }
  }
  static MeshPtr dummy(std::make_shared<Mesh>());
  return dummy;
}

/**
* 立方体を作成する.
*
* @param 立方体のメッシュ名.
*/
void Buffer::CreateCube(const char* name)
{
  /*
       6---7
      /|  /|
     / 5-/-4
    3---2 /
    |/  |/
    0---1
  */
  static const glm::vec3 basePositions[] = {
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
    { 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1},
  };
  static const glm::vec2 baseTexCoords[] = { { 0, 1}, { 0, 0}, { 1, 0}, { 1, 1} };
  static const GLubyte baseIndices[] = { 0, 1, 2, 2, 3, 0 };
  static const glm::vec3 normals[] = {
    { 0, 0, 1}, { 1, 0, 0}, { 0, 0,-1}, {-1, 0, 0}, { 0, -1, 0}, { 0, 1, 0} };
  static const int planes[6][4] = {
    { 0, 1, 2, 3}, { 1, 4, 7, 2}, { 4, 5, 6, 7}, { 5, 0, 3, 6},
    { 5, 4, 1, 0}, { 3, 2, 7, 6} };

  Vertex v;
  v.color = glm::vec4(1);
  std::vector<Vertex> vertices;
  std::vector<GLubyte> indices;
  for (size_t plane = 0; plane < 6; ++plane) {
    for (size_t i = 0; i < 4; ++i) {
      v.position = basePositions[planes[plane][i]];
      v.texCoord = baseTexCoords[i];
      v.normal = normals[plane];
      vertices.push_back(v);
    }
    for (size_t i = 0; i < 6; ++i) {
      indices.push_back(static_cast<GLubyte>(baseIndices[i] + plane * 4));
    }
  }
  const size_t vOffset = AddVertexData(vertices.data(), vertices.size() * sizeof(Vertex));
  const size_t iOffset = AddIndexData(indices.data(), indices.size() * sizeof(GLubyte));

  AddMesh(name, indices.size(), GL_UNSIGNED_BYTE, iOffset, vOffset);
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

  AddMesh(name, indices.size(), GL_UNSIGNED_BYTE, iOffset, vOffset);
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

  AddMesh(name, indices.size(), GL_UNSIGNED_SHORT, iOffset, vOffset);
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

/**
* アクセッサが指定するバイナリデータの位置とバイト数を取得する.
*
* @param accessor    glTFアクセッサ
* @param bufferViews バイナリデータを分割管理するためのデータ配列.
* @param binFiles    バイナリファイルの配列.
* @param pp          取得したバイナリデータの位置.
* @param pLength     取得したバイナリデータのバイト数.
* @param pStride     取得したバイナリデータのデータ幅(頂点データの定義で使用).
*/
void GetBuffer(const json11::Json& accessor, const json11::Json& bufferViews, const std::vector<std::vector<char>>& binFiles, const void** pp, size_t* pLength, int* pStride = nullptr)
{
  const int bufferViewId = accessor["bufferView"].int_value();
  const int byteOffset = accessor["byteOffset"].int_value();
  const json11::Json bufferView = bufferViews[bufferViewId];
  const int bufferId = bufferView["buffer"].int_value();
  const int baesByteOffset = bufferView["byteOffset"].int_value();
  int byteLength = bufferView["byteLength"].int_value();
  if (!accessor["count"].is_null()) {
    int unitByteSize;
    switch (accessor["componentType"].int_value()) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
      unitByteSize = 1;
      break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
      unitByteSize = 2;
      break;
    default:
      unitByteSize = 4;
      break;
    }
    const std::string& type = accessor["type"].string_value();
    static const char* const typeNameList[] = { "SCALAR", "VEC2", "VEC3", "VEC4", "MAT4" };
    static const int typeSizeList[] = { 1, 2, 3, 4, 16 };
    int typeSize = -1;
    for (size_t i = 0; i < 5; ++i) {
      if (type == typeNameList[i]) {
        typeSize = typeSizeList[i];
        break;
      }
    }
    if (typeSize < 0) {
      std::cerr << "ERROR in GetBuffer: アクセサの型(" << type << ")には対応していません.\n";
      byteLength = 0;
    } else {
      byteLength = std::min(byteLength, accessor["count"].int_value() * unitByteSize * typeSize);
    }
  }

  if (pStride) {
    *pStride = bufferView["byteStride"].int_value();
  }

  *pLength = byteLength;
  *pp = binFiles[bufferId].data() + baesByteOffset + byteOffset;
}

/**
* 頂点属性を設定する.
*
* @param prim        頂点データを設定するプリミティブ.
* @param index       設定する頂点属性のインデックス.
* @param accessor    頂点データの格納情報.
* @param bufferViews 頂点データを参照するためのバッファ・ビュー配列.
* @param binFiles    頂点データを格納しているバイナリデータ配列.
*
* @retval true  設定成功.
* @retval false 設定失敗.
*/
bool Buffer::SetAttribute(
  Primitive& prim, int index, const json11::Json& accessor, const json11::Json& bufferViews, const std::vector<std::vector<char>>& binFiles)
{
  if (accessor.is_null()) {
    return true;
  }
  static const char* const typeNameList[] = { "SCALAR", "VEC2", "VEC3", "VEC4" };
  static const int typeSizeList[] = { 1, 2, 3, 4 };
  const std::string& type = accessor["type"].string_value();
  int size = -1;
  for (size_t i = 0; i < 4; ++i) {
    if (type == typeNameList[i]) {
      size = typeSizeList[i];
      break;
    }
  }
  if (size < 0) {
    std::cerr << "[エラー]" << __func__ << ": " << type << "は頂点属性に設定できません.\n";
    return false;
  }

  const void* p;
  size_t byteLength;
  int byteStride;
  GetBuffer(accessor, bufferViews, binFiles, &p, &byteLength, &byteStride);
  const GLenum componentType = accessor["componentType"].int_value();
  prim.vao->Bind();
  prim.vao->VertexAttribPointer(index, size, componentType, GL_FALSE, byteStride, vboEnd);
  prim.vao->Unbind();

  vbo.BufferSubData(vboEnd, byteLength, p);
  vboEnd += ((byteLength + 3) / 4) * 4; // 4バイト境界に整列.

  return true;
}

/**
*
*/
bool Buffer::LoadMesh(const char* path)
{
  // gltfファイルを読み込む.
  std::vector<char> gltfFile = ReadFromFile(path);
  if (gltfFile.empty()) {
    return false;
  }
  gltfFile.push_back('\0');

  // json解析.
  std::string err;
  const json11::Json json = json11::Json::parse(gltfFile.data(), err);
  if (!err.empty()) {
    std::cerr << "ERROR: " << path << "の読み込みに失敗しました.\n";
    std::cerr << err << "\n";
    return false;
  }

  // バイナリファイルを読み込む.
  std::vector<std::vector<char>> binFiles;
  for (const json11::Json& e : json["buffers"].array_items()) {
    const json11::Json& uri = e["uri"];
    if (!uri.is_string()) {
      std::cerr << "[エラー]" << __func__ << ": " << path << "に不正なuriがあります.\n";
      return false;
    }
    const std::string binPath = std::string("Res/") + uri.string_value();
    binFiles.push_back(ReadFromFile(binPath.c_str()));
    if (binFiles.back().empty()) {
      return false;
    }
  }

  FilePtr pFile = std::make_shared<File>();
  File& file = *pFile;

  // アクセッサからデータを取得してGPUへ転送.
  const json11::Json& accessors = json["accessors"];
  const json11::Json& bufferViews = json["bufferViews"];

  // インデックスデータと頂点属性データのアクセッサIDを取得.
  file.meshes.reserve(json["meshes"].array_items().size());
  for (const auto& currentMesh : json["meshes"].array_items()) {
    MeshData mesh;
    mesh.name = currentMesh["name"].string_value();
    const std::vector<json11::Json>& primitives = currentMesh["primitives"].array_items();
    mesh.primitives.resize(primitives.size());
    for (size_t primId = 0; primId < primitives.size(); ++primId) {
      const json11::Json& primitive = currentMesh["primitives"][primId];

      // 頂点インデックス.
      {
        const int accessorId_index = primitive["indices"].int_value();
        const json11::Json& accessor = accessors[accessorId_index];
        if (accessor["type"].string_value() != "SCALAR") {
          std::cerr << "ERROR: インデックスデータ・タイプはSCALARでなくてはなりません \n";
          std::cerr << "  type = " << accessor["type"].string_value() << "\n";
          return false;
        }

        mesh.primitives[primId].mode = primitive["mode"].is_null() ? GL_TRIANGLES : primitive["mode"].int_value();
        mesh.primitives[primId].count = accessor["count"].int_value();
        mesh.primitives[primId].type = accessor["componentType"].int_value();
        mesh.primitives[primId].indices = reinterpret_cast<const GLvoid*>(iboEnd);

        const void* p;
        size_t byteLength;
        GetBuffer(accessor, bufferViews, binFiles, &p, &byteLength);
        ibo.BufferSubData(iboEnd, byteLength, p);
        iboEnd += ((byteLength + 3) / 4) * 4; // 次に来るのがどのデータ型でも大丈夫なように4バイト境界に整列.
      }

      // 頂点属性.
      const json11::Json& attributes = primitive["attributes"];
      const int accessorId_position = attributes["POSITION"].int_value();
      const int accessorId_normal = attributes["NORMAL"].is_null() ? -1 : attributes["NORMAL"].int_value();
      const int accessorId_texcoord = attributes["TEXCOORD_0"].is_null() ? -1 : attributes["TEXCOORD_0"].int_value();

      mesh.primitives[primId].vao = std::make_shared<VertexArrayObject>();
      mesh.primitives[primId].vao->Create(vbo.Id(), ibo.Id());
      SetAttribute(mesh.primitives[primId], 0, accessors[accessorId_position], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 2, accessors[accessorId_texcoord], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 3, accessors[accessorId_normal], bufferViews, binFiles);

      mesh.primitives[primId].material = primitive["material"].int_value();
    }
    file.meshes.push_back(mesh);
  }

  // マテリアル.
  {
    const std::vector<json11::Json> materials = json["materials"].array_items();
    file.materials.reserve(materials.size());
    for (const json11::Json& material : materials) {
      std::string texturePath;
      const json11::Json& pbr = material["pbrMetallicRoughness"];
      const json11::Json& index = pbr["baseColorTexture"]["index"];
      if (index.is_number()) {
        const int textureId = index.int_value();
        const json11::Json& texture = json["textures"][textureId];
        const int imageSourceId = texture["source"].int_value();
        const json11::Json& imageName = json["images"][imageSourceId]["name"];
        if (imageName.is_string()) {
          texturePath = std::string("Res/") + imageName.string_value() + ".tga";
        }
      }
      glm::vec4 col(0, 0, 0, 1);
      const std::vector<json11::Json>& baseColorFactor = pbr["baseColorFactor"].array_items();
      if (baseColorFactor.size() >= 4) {
        for (size_t i = 0; i < 4; ++i) {
          col[i] = static_cast<float>(baseColorFactor[i].number_value());
        }
      }
      Texture::Image2DPtr tex;
      if (!texturePath.empty()) {
        tex = Texture::Image2D::Create(texturePath.c_str());
      }
      file.materials.push_back(CreateMaterial(col, tex));
    }
  }

  file.name = path;
  files.insert(std::make_pair(file.name, pFile));
  std::cout << "[INFO]" << __func__ << ": '" << path << "'を読み込みました.\n";
  for (size_t i = 0; i < file.meshes.size(); ++i) {
    std::cout << "  mesh[" << i << "] = " << file.meshes[i].name << "\n";
  }

  return true;
}

/**
* シェーダにビュー・プロジェクション行列を設定する.
*
* @param matVP ビュー・プロジェクション行列.
*
* @note こういう処理はUBOで共有するとかのほうがよい設計だと思う.
*/
void Buffer::SetViewProjectionMatrix(const glm::mat4& matVP) const
{
  progStaticMesh->Use();
  progStaticMesh->SetViewProjectionMatrix(matVP);
  progSkeletalMesh->Use();
  progSkeletalMesh->SetViewProjectionMatrix(matVP);
  glUseProgram(0);
}

} // namespace Mesh