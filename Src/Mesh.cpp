/**
* @file Mesh.h
*/
#define NOMINMAX
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Mesh {

/**
* メッシュを描画する.
*
* 事前にVAO、シェーダー、テクスチャ等をバインドしておくこと.
*/
void Mesh::Draw() const
{
  if (!file || !file->vao) {
    return;
  }
  const MeshData& meshData = file->meshes[meshId];
  GLuint prevTexId = 0;
  for (const auto& prim : meshData.primitives) {
    file->vao->ResetVertexAttribPointer();
    bool hasColorAttr = false;
    for (const auto& attr : prim.attributes) {
      if (attr.index == GL_INVALID_INDEX) {
        continue;
      }
      if (attr.index == 1) {
        hasColorAttr = true;
      }
      file->vao->VertexAttribPointer(attr.index, attr.size, attr.type, GL_FALSE, attr.stride, attr.offset);
    }
    if (!hasColorAttr) {
      static const glm::vec4 color(1);
      glVertexAttrib4fv(1, &color.x);
    }

    if (prim.material >= 0 && prim.material < static_cast<int>(file->materials.size())) {
      const Material& m = file->materials[prim.material];
      if (m.texture) {
        const GLuint texId = m.texture->Id();
        if (prevTexId != texId) {
          //program->BindTexture(0, texId);
          prevTexId = texId;
        }
      }
      //program->SetMaterialColor(m.baseColor);
    }
    glDrawElementsBaseVertex(prim.mode, prim.count, prim.type, prim.indices, prim.baseVertex);
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
  FilePtr pFile = std::make_shared<File>();
  pFile->name = mesh.name;
  pFile->meshes.resize(1);
  pFile->meshes[0].name = mesh.name;
  pFile->meshes[0].primitives.resize(1);
  pFile->meshes[0].primitives[0].mode = mesh.mode;
  pFile->meshes[0].primitives[0].count = mesh.count;
  pFile->meshes[0].primitives[0].type = mesh.type;
  pFile->meshes[0].primitives[0].indices = mesh.indices;
  pFile->meshes[0].primitives[0].baseVertex = mesh.baseVertex;
  pFile->meshes[0].primitives[0].attributes[0] = { 0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, position) };
  pFile->meshes[0].primitives[0].attributes[1] = { 1, 4, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, color) };
  pFile->meshes[0].primitives[0].attributes[2] = { 2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, texCoord) };
  pFile->meshes[0].primitives[0].attributes[3] = { 3, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, normal) };
  pFile->meshes[0].primitives[0].material = 0;
  pFile->materials.resize(1);
  pFile->materials[0].baseColor = glm::vec4(1);
  pFile->vao = &vao;
  files.insert(std::make_pair(pFile->name, pFile));
  meshes.insert(std::make_pair(pFile->meshes[0].name, MeshIndex{ pFile, 0 }));
  std::cout << "Mesh::Buffer: メッシュ'" << mesh.name << "'を追加.\n";
}

/**
* メッシュを取得する.
*
* @param meshName 取得したいメッシュの名前.
*
* @return meshNameと同じ名前を持つメッシュ.
*/
Mesh Buffer::GetMesh(const char* meshName) const
{
  const auto itr = meshes.find(meshName);
  if (itr == meshes.end()) {
    static const Mesh dummy;
    return dummy;
  }
  return Mesh(itr->second.file, itr->second.index);
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
*
*/
glm::vec3 GetVec3(const json11::Json& json)
{
  const std::vector<json11::Json>& a = json.array_items();
  if (a.size() < 3) {
    return glm::vec3(0);
  }
  return glm::vec3(
    static_cast<float>(a[0].number_value()),
    static_cast<float>(a[1].number_value()),
    static_cast<float>(a[2].number_value())
  );
}

/**
*
*/
glm::quat GetQuat(const json11::Json& json)
{
  const std::vector<json11::Json>& a = json.array_items();
  if (a.size() < 4) {
    return glm::quat(0, 0, 0, 1);
  }
  return glm::quat(
    static_cast<float>(a[3].number_value()),
    static_cast<float>(a[0].number_value()),
    static_cast<float>(a[1].number_value()),
    static_cast<float>(a[2].number_value())
  );
}

/**
*
*/
glm::mat4 GetMat4(const json11::Json& json)
{
  const std::vector<json11::Json>& a = json.array_items();
  if (a.size() < 16) {
    return glm::mat4(1);
  }
  return glm::mat4(
    a[0].number_value(), a[1].number_value(), a[2].number_value(), a[3].number_value(),
    a[4].number_value(), a[5].number_value(), a[6].number_value(), a[7].number_value(),
    a[8].number_value(), a[9].number_value(), a[10].number_value(), a[11].number_value(),
    a[12].number_value(), a[13].number_value(), a[14].number_value(), a[15].number_value()
  );
}

/**
*
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
*
*/
bool Buffer::SetAttribute(
  Primitive& prim, const json11::Json& accessor, const json11::Json& bufferViews, std::vector<std::vector<char>>& binFiles, int index, int size)
{
  if (accessor.is_null()) {
    return true;
  }
  static const char* const typeNameList[] = { "", "SCALAR", "VEC2", "VEC3", "VEC4" };
  VertexAttribute& attr = prim.attributes[index];
  if (accessor["type"].string_value() != typeNameList[size]) {
    std::cerr << "ERROR: データは" << typeNameList[size] << "でなくてはなりません \n";
    std::cerr << "  type = " << accessor["type"].string_value() << "\n";
    return false;
  }

  const void* p;
  size_t byteLength;
  int byteStride;
  GetBuffer(accessor, bufferViews, binFiles, &p, &byteLength, &byteStride);

  attr.index = index;
  attr.size = size;
  attr.type = accessor["componentType"].int_value();
  attr.stride = byteStride;
  attr.offset = vboEnd;

  vbo.BufferSubData(vboEnd, byteLength, p);
  vboEnd += ((byteLength + 3) / 4) * 4;

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
  const json11::Json& buffers = json["buffers"][0]["uri"];
  if (!buffers.is_string()) {
    std::cerr << "ERROR: " << path << "にbuffers:uri属性が見つかりません.\n";
    return false;
  }
  std::vector<std::vector<char>> binFiles;
  const std::string binPath = std::string("Res/") + buffers.string_value();
  binFiles.push_back(ReadFromFile(binPath.data()));
  if (binFiles.back().empty()) {
    return false;
  }

  FilePtr pFile = std::make_shared<File>();
  File& file = *pFile;

  // アクセッサからデータを取得してGPUへ転送.
  const json11::Json& accessors = json["accessors"];
  const json11::Json& bufferViews = json["bufferViews"];

  // インデックスデータと頂点属性データのアクセッサIDを取得.
  // とりあえず必要分のみということで、座標と法線のみ取っておく.
  file.meshes.reserve(json["meshes"].array_items().size());
  for (const auto& currentMesh : json["meshes"].array_items()) {
    MeshData mesh;
    mesh.name = currentMesh["name"].string_value();
    const std::vector<json11::Json>& primitives = currentMesh["primitives"].array_items();
    mesh.primitives.resize(primitives.size());
    for (size_t primId = 0; primId < primitives.size(); ++primId) {
      const json11::Json& primitive = currentMesh["primitives"][primId];
      const json11::Json& attributes = primitive["attributes"];
      const int accessorId_index = primitive["indices"].int_value();
      const int accessorId_position = attributes["POSITION"].int_value();
      const int accessorId_normal = attributes["NORMAL"].is_null() ? -1 : attributes["NORMAL"].int_value();
      const int accessorId_texcoord = attributes["TEXCOORD_0"].is_null() ? -1 : attributes["TEXCOORD_0"].int_value();
      const int accessorId_weights = attributes["WEIGHTS_0"].is_null() ? -1 : attributes["WEIGHTS_0"].int_value();
      const int accessorId_joints = attributes["JOINTS_0"].is_null() ? -1 : attributes["JOINTS_0"].int_value();

      // 頂点インデックス.
      {
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
        iboEnd += ((byteLength + 3) / 4) * 4;
      }

      // 頂点属性.
      SetAttribute(mesh.primitives[primId], accessors[accessorId_position], bufferViews, binFiles, 0, 3);
      SetAttribute(mesh.primitives[primId], accessors[accessorId_texcoord], bufferViews, binFiles, 2, 2);
      SetAttribute(mesh.primitives[primId], accessors[accessorId_normal], bufferViews, binFiles, 3, 3);
      SetAttribute(mesh.primitives[primId], accessors[accessorId_weights], bufferViews, binFiles, 4, 4);
      SetAttribute(mesh.primitives[primId], accessors[accessorId_joints], bufferViews, binFiles, 5, 4);

      mesh.primitives[primId].material = primitive["material"].int_value();
    }
    file.meshes.push_back(mesh);
  }

  // マテリアル.
  {
    const std::vector<json11::Json> materials = json["materials"].array_items();
    file.materials.reserve(materials.size());
    for (const auto& material : materials) {
      Material m;
      const json11::Json& index = material["pbrMetallicRoughness"]["baseColorTexture"]["index"];
      if (index.is_number()) {
        const int textureId = index.int_value();
        const json11::Json& texture = json["textures"][textureId];
        const int imageSourceId = texture["source"].int_value();
        const json11::Json& imageUri = json["images"][imageSourceId]["uri"];
        if (imageUri.is_string()) {
          const std::string str = std::string("Res/") + imageUri.string_value();
          m.texture = std::make_shared<Texture::Image2D>(str.c_str());
        }
      }
      const std::vector<json11::Json>& baseColorFactor = material["pbrMetallicRoughness"]["baseColorFactor"].array_items();
      if (baseColorFactor.size() >= 4) {
        for (size_t i = 0; i < 4; ++i) {
          m.baseColor[i] = static_cast<float>(baseColorFactor[i].number_value());
        }
      }
      file.materials.push_back(m);
    }
  }

  file.name = path;
  file.vao = &vao;
  files.insert(std::make_pair(file.name, pFile));
  for (size_t i = 0; i < file.meshes.size(); ++i) {
    meshes.insert(std::make_pair(file.meshes[i].name, MeshIndex{ pFile, i }));
  }

  return true;
}

} // namespace Mesh