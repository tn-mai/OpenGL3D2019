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

// �X�L���f�[�^.
struct Skin {
  std::string name;
  std::vector<int> joints;
};

// �m�[�h.
struct Node {
  Node* parent = nullptr;
  int mesh = -1;
  int skin = -1;
  std::vector<Node*> children;
  glm::aligned_mat4 matLocal = glm::aligned_mat4(1);
  glm::aligned_mat4 matGlobal = glm::aligned_mat4(1);
  glm::aligned_mat4 matInverseBindPose = glm::aligned_mat4(1);
};

// �A�j���[�V�����̃L�[�t���[��.
template<typename T>
struct KeyFrame {
  float frame;
  T value;
};

// �A�j���[�V�����̃^�C�����C��.
template<typename T>
struct Timeline {
  int targetNodeId;
  std::vector<KeyFrame<T>> timeline;
};

// �A�j���[�V����.
struct Animation {
  std::vector<Timeline<glm::aligned_vec3>> translationList;
  std::vector<Timeline<glm::aligned_quat>> rotationList;
  std::vector<Timeline<glm::aligned_vec3>> scaleList;
  float totalTime = 0;
  std::string name;
};

// �V�[��.
struct Scene {
  int rootNode;
  std::vector<const Node*> meshNodes;
};

// �t�@�C��.
struct File {
  std::string name; // �t�@�C����.
  std::vector<Scene> scenes;
  std::vector<Node> nodes;
  std::vector<MeshData> meshes;
  std::vector<Material> materials;
  std::vector<Skin> skins;
  std::vector<Animation> animations;
};
using FilePtr = std::shared_ptr<File>;

struct MeshTransformation {
  std::vector<glm::aligned_mat4> transformations;
  std::vector<glm::aligned_mat4> matRoot;
};

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
  Mesh(Buffer* p, const FilePtr& f, const Node* n) : parent(p), file(f), node(n) {}
  void Draw() const;
  void Update(float deltaTime);
  void SetAnimation(int);
  void SetAnimation(const char*);
  int GetAnimation() const;
  const std::string& GetAnimationName() const;
  bool IsFinishAnimation() const;
  size_t GetAnimationCount() const;

  std::string name;

//  GLenum mode = GL_TRIANGLES;
//  GLsizei count = 0;
//  GLenum type = GL_UNSIGNED_SHORT;
//  const GLvoid* indices = 0;
//  GLint baseVertex = 0;
  
  Buffer* parent = nullptr;

  FilePtr file;
  const Node* node = nullptr;
  const Animation* animation = nullptr;

  glm::aligned_vec3 translation = glm::vec3(0);
  glm::aligned_quat rotation = glm::quat(glm::vec3(0));
  glm::aligned_vec3 scale = glm::vec3(1);
  glm::aligned_vec4 color = glm::vec4(1);

  float frame = 0;
  bool isLoop = true;

  GLintptr uboOffset = 0;
  GLsizeiptr uboSize = 0;
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
  SkeletalMeshPtr GetSkeletalMesh(const char* meshName) const;
  void Bind();
  void Unbind();

  void SetViewProjectionMatrix(const glm::mat4&) const;

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
    const Node* node = nullptr;
  };
  std::unordered_map<std::string, MeshIndex> meshes;
  std::unordered_map<std::string, FilePtr> files;

  Shader::ProgramPtr progStaticMesh;
  Shader::ProgramPtr progSkeletalMesh;

  bool SetAttribute(
    Primitive& prim, int index, const json11::Json& accessor, const json11::Json& bufferViews, const std::vector<std::vector<char>>& binFiles);
};

} // namespace Mesh

#endif // MESH_H_INCLUDED