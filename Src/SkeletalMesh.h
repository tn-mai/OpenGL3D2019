/**
* @file SleletalMesh.h
*/
#ifndef SKELETAL_MESH_H_INCLUDED
#define SKELETAL_MESH_H_INCLUDED
#include "Mesh.h"

namespace Mesh {

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

// �g���t�@�C��.
struct ExtendedFile {
  std::string name; // �t�@�C����.
  std::vector<MeshData> meshes;
  std::vector<Material> materials;

  std::vector<Scene> scenes;
  std::vector<Node> nodes;
  std::vector<Skin> skins;
  std::vector<Animation> animations;
};

/**
* ���i�A�j���[�V���������郁�b�V��.
*
* �A�j���[�V��������@�\���܂ނ��߁A�C���X�^���X�͂��̓s�x�쐬������𓾂Ȃ�.
*/
class SkeletalMesh
{
public:
  enum class State {
    stop, ///< ��~��.
    play, ///< �Đ���.
    pause, ///< �ꎞ��~��.
  };

  SkeletalMesh() = default;
  SkeletalMesh(const ExtendedFilePtr& f, const Node* n);

  void Update(float deltaTime, const glm::aligned_mat4& matModel, const glm::vec4& color);
  void Draw() const;
  const std::vector<Animation>& GetAnimationList() const;
  const std::string& GetAnimation() const;
  float GetTotalAnimationTime() const;
  State GetState() const;
  bool Play(const std::string& name, bool loop = true);
  bool Stop();
  bool Pause();
  bool Resume();
  void SetAnimationSpeed(float speed);
  float GetAnimationSpeed() const;
  void SetPosition(float);
  float GetPosition() const;
  bool IsFinished() const;
  bool Loop() const;
  void Loop(bool);

private:
  std::string name;
  ExtendedFilePtr file;
  const Node* node = nullptr;
  const Animation* animation = nullptr;

  State state = State::stop;
  float frame = 0;
  float animationSpeed = 1;
  bool loop = true;

  GLintptr uboOffset = 0;
  GLsizeiptr uboSize = 0;
};
using SkeletalMeshPtr = std::shared_ptr<SkeletalMesh>;

void GetMeshNodeList(const Node* node, std::vector<const Node*>& list);

/**
* SkeletalMesh�̎g����:
*
* main�֐��ւ̒ǉ�:
* 1. main�֐��́AOpenGL�̏������������������ƁA�V�[�����쐬������O��Mesh::GlobalSkeletalMeshState::Initialize()�����s.
* 2. main�֐��̏I�����O��Mesh::GlobalSkeletalMeshState::Finalize()�����s.
* 3. SceneStack::Update�֐��Ăяo���̒��O��Mesh::GlobalSkeletalMeshState::ResetUniformData()�����s.
* 4. SceneStack::Update�֐��Ăяo���̒����Mesh::GlobalSkeletalMeshState::UploadUniformData()�����s.
*
* Mesh::Buffer�N���X�ւ̒ǉ�:
* 1. Mesh::Material�\���̂ɃX�P���^�����b�V���p�V�F�[�_�|�C���^��ǉ�.
* 2. Mesh::Buffer�N���X�ɃX�P���^�����b�V���p�̃V�F�[�_��ǂݍ��ޏ�����ǉ�.
* 3. Mesh::Buffer::Init�֐��̍Ō��BindUniformBlock�����s.
* 4. Mesh::Buffer::LoadMesh�֐����X�P���^�����b�V���Ή��łɍ����ւ���.
* 5. Mesh::Buffer�N���X��LoadSkeletalMesh�֐��AGetSkeletalMesh�֐���ǉ�.
*
* �Q�[����:
* 
*/
namespace GlobalSkeletalMeshState {

bool Initialize();
void Finalize();
bool BindUniformBlock(const Shader::ProgramPtr&);
void ResetUniformData();
void UploadUniformData();

} // namespace GlobalSkeletalMeshState

} // namespace Mesh

#endif // SKELETAL_MESH_H_INCLUDED