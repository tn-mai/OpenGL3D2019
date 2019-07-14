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

// ��s�錾.
class Buffer;
using BufferPtr = std::shared_ptr<Buffer>;
struct Mesh;
using MeshPtr = std::shared_ptr<Mesh>;
class SkeletalMesh;
using SkeletalMeshPtr = std::shared_ptr<SkeletalMesh>;

// �X�P���^�����b�V���p.
struct Node;
struct ExtendedFile;
using ExtendedFilePtr = std::shared_ptr<ExtendedFile>;

// ���_�f�[�^.
struct Vertex
{
  glm::vec3 position;
  glm::vec4 color;
  glm::vec2 texCoord;
  glm::vec3 normal;
  GLfloat   weights[4];
  GLushort  joints[4];
};

// �}�e���A��.
struct Material {
  glm::vec4 baseColor = glm::vec4(1);
  Texture::Image2DPtr texture;
  Shader::ProgramPtr program;
  Shader::ProgramPtr progSkeletalMesh;
};

// ���b�V���v���~�e�B�u.
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

// ���b�V���f�[�^.
struct MeshData {
  std::string name;
  std::vector<Primitive> primitives;
};

// �t�@�C��.
struct File {
  std::string name; // �t�@�C����.
  std::vector<MeshData> meshes;
  std::vector<Material> materials;
};
using FilePtr = std::shared_ptr<File>;

/**
* �`�悷�郁�b�V���f�[�^.
*
* Primitve���܂Ƃ߂�����.
* �V�[�����[�g�m�[�h���烁�b�V���m�[�h�܂ł̕ϊ��s����|���Ȃ��ƁA�������p���ŕ`�悳��Ȃ��ꍇ������.
* �A�j���[�V���������̂��{�[���m�[�h�����Ȃ�A����͈�x�v�Z����Ώ\��.
* �����A�ʏ�m�[�h���A�j���[�V�����������Ă���ꍇ������̂ŁA���̏ꍇ�͖��񃋁[�g�m�[�h����Čv�Z����K�v������.
* glTF�̃A�j���[�V�����f�[�^�ɂ͒ʏ�m�[�h�ƃ{�[���m�[�h�̋�ʂ��Ȃ��̂ŁA��ʂ�������΃A�j���[�V�����f�[�^�̑ΏۂƂȂ�m�[�h��
* �X�P���g���m�[�h���X�g�Ɋ܂܂�邩�ǂ����Ŕ��肷��K�v�����肻��.
*
* �쐬���@:
* - Buffer::GetMesh�ō쐬.
*
* �X�V���@:
* -# Buffer::ResetUniformData()��UBO�o�b�t�@�����Z�b�g.
* -# �`�悷�邷�ׂẴ��b�V���ɂ��āAMesh::Update�ŃA�j���[�V������UBO�o�b�t�@�̍X�V(����͕������ق�����������).
* -# Buffer::UploadUniformData()��UBO�o�b�t�@��VRAM�ɓ]��.
*
* �`����@:
* - Mesh::Draw�ŕ`��.
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
* �S�Ă̕`��f�[�^��ێ�����N���X.
*
* ���b�V���̓o�^���@:
* - ���_�f�[�^�ƃC���f�b�N�X�f�[�^��p�ӂ���.
* - Mesh�I�u�W�F�N�g��p�ӂ��A���b�V���������߂�Mesh::name�ɐݒ肷��.
* - AddVertexData()�Œ��_�̍��W�A�e�N�X�`�����W�A�F�Ȃǂ�ݒ肵�A�߂�l��Vertex�T�C�Y�Ŋ������l��baseVertex�ɐݒ肷��.
* - AddIndexData()�ŃC���f�b�N�X�f�[�^��ݒ肵�A�߂�l��Mesh::indices�ɐݒ肷��.
* - Mesh��mode, count, type�����o�[�ɓK�؂Ȓl��ݒ肷��.
*   ���Ƃ��΍��W�𒸓_�A�g���r���[�g��0�ԖڂƂ��Ďw�肷��ꍇ�AMesh::attributes[0]::offset�ɐݒ肷��.
* - ���_�A�g���r���[�g��index, size, type, stride�ɓK�؂Ȓl��ݒ肷��.
* - AddMesh()�Ń��b�V����o�^����.
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

  // �X�P���^�����b�V���p.
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

  // �X�P���^�����b�V���p.
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