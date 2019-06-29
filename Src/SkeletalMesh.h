/**
* @file SleletalMesh.h
*/
#ifndef SKELETAL_MESH_H_INCLUDED
#define SKELETAL_MESH_H_INCLUDED
#include "Mesh.h"

namespace Mesh {


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
* 5. Mesh::Buffer�N���X��GetSkeletalMesh�֐���ǉ�.
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

GLintptr PushUniformData(const void* data, size_t size);
void BindUniformData(GLintptr offset, GLsizeiptr size);

} // namespace GlobalSkeletalMeshState

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
  SkeletalMesh(Buffer* p, const FilePtr& f, const Node* n);

  void Update(float deltaTime, const glm::aligned_mat4& matModel, const glm::vec4& color);
  void Draw() const;
  const std::vector<Animation>& GetAnimationList() const;
  const std::string& GetAnimation() const;
  State GetState() const;
  bool Play(const char*, bool doesLoop = true);
  bool Stop();
  bool Pause();
  bool Resume();
  void SetAnimationSpeed(float speed);
  float GetAnimationSpeed() const;

private:
  std::string name;
  Buffer* parent = nullptr;
  FilePtr file;
  const Node* node = nullptr;
  const Animation* animation = nullptr;

  State state = State::stop;
  float frame = 0;
  float animationSpeed = 1;
  bool doesLoop = true;

  GLintptr uboOffset = 0;
  GLsizeiptr uboSize = 0;
};
using SkeletalMeshPtr = std::shared_ptr<SkeletalMesh>;

/**
* �X�P���^���E���b�V���`��pUBO�f�[�^.
*/
struct alignas(256) UniformDataMeshMatrix
{
  glm::aligned_vec4 color;
  glm::aligned_mat3x4 matModel[4]; // it must transpose.
  glm::aligned_mat3x4 matNormal[4]; // w isn't ussing. no need to transpose.
  glm::aligned_mat3x4 matBones[256]; // it must transpose.
};

/**
* �A�j���[�V�����p�̒��ԃf�[�^.
*/
struct AnimatedNodeTree {
  struct Transformation {
    glm::aligned_vec3 translation = glm::aligned_vec3(0);
    glm::aligned_quat rotation = glm::aligned_quat(0, 0, 0, 1);
    glm::aligned_vec3 scale = glm::aligned_vec3(1);
    glm::aligned_mat4 matLocal = glm::aligned_mat4(1);
    glm::aligned_mat4 matGlobal = glm::aligned_mat4(1);
    bool isCalculated = false;
    bool hasTransformation = false;
  };
  std::vector<Transformation> nodeTransformations;
};

void GetMeshNodeList(const Node* node, std::vector<const Node*>& list);
void CalcGlobalTransform(const std::vector<Node>& nodes, const Node& node, AnimatedNodeTree& animated);
AnimatedNodeTree MakeAnimatedNodeTree(const File& file, const Animation& animation, float keyFrame);
glm::aligned_mat4 DecomposeRotation(const glm::aligned_mat4& m);
MeshTransformation CalculateTransform(const FilePtr&, const Node*, const Animation*, float);

/**
*
*/
template<typename T>
T Interporation(const Timeline<T>& data, float frame)
{
  const auto maxFrame = std::lower_bound(data.timeline.begin(), data.timeline.end(), frame,
    [](const KeyFrame<T>& keyFrame, float frame) { return keyFrame.frame < frame; });
  if (maxFrame == data.timeline.begin()) {
    return data.timeline.front().value;
  }
  if (maxFrame == data.timeline.end()) {
    return data.timeline.back().value;
  }
  const auto minFrame = maxFrame - 1;
  const float ratio = glm::clamp((frame - minFrame->frame) / (maxFrame->frame - minFrame->frame), 0.0f, 1.0f);
  return glm::mix(minFrame->value, maxFrame->value, ratio);
}

/**
*
*/
template<typename T, glm::qualifier Q>
glm::qua<T, Q> Interporation(const Timeline<glm::qua<T, Q> >& data, float frame)
{
  const auto maxFrame = std::lower_bound(data.timeline.begin(), data.timeline.end(), frame,
    [](const KeyFrame<glm::qua<T, Q>>& keyFrame, float frame) { return keyFrame.frame < frame; });
  if (maxFrame == data.timeline.begin()) {
    return data.timeline.front().value;
  }
  if (maxFrame == data.timeline.end()) {
    return data.timeline.back().value;
  }
  const auto minFrame = maxFrame - 1;
  const float ratio = glm::clamp((frame - minFrame->frame) / (maxFrame->frame - minFrame->frame), 0.0f, 1.0f);
  return glm::slerp(minFrame->value, maxFrame->value, ratio);
}

} // namespace Mesh

#endif // SKELETAL_MESH_H_INCLUDED