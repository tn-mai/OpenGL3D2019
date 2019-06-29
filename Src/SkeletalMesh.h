/**
* @file SleletalMesh.h
*/
#ifndef SKELETAL_MESH_H_INCLUDED
#define SKELETAL_MESH_H_INCLUDED
#include "Mesh.h"

namespace Mesh {


/**
* SkeletalMeshの使い方:
*
* main関数への追加:
* 1. main関数の、OpenGLの初期化が完了したあと、シーンを作成するより前にMesh::GlobalSkeletalMeshState::Initialize()を実行.
* 2. main関数の終了直前にMesh::GlobalSkeletalMeshState::Finalize()を実行.
* 3. SceneStack::Update関数呼び出しの直前にMesh::GlobalSkeletalMeshState::ResetUniformData()を実行.
* 4. SceneStack::Update関数呼び出しの直後にMesh::GlobalSkeletalMeshState::UploadUniformData()を実行.
*
* Mesh::Bufferクラスへの追加:
* 1. Mesh::Material構造体にスケルタルメッシュ用シェーダポインタを追加.
* 2. Mesh::Bufferクラスにスケルタルメッシュ用のシェーダを読み込む処理を追加.
* 3. Mesh::Buffer::Init関数の最後でBindUniformBlockを実行.
* 4. Mesh::Buffer::LoadMesh関数をスケルタルメッシュ対応版に差し替える.
* 5. Mesh::BufferクラスにGetSkeletalMesh関数を追加.
*
* ゲーム中:
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
* 骨格アニメーションをするメッシュ.
*
* アニメーション制御機能を含むため、インスタンスはその都度作成せざるを得ない.
*/
class SkeletalMesh
{
public:
  enum class State {
    stop, ///< 停止中.
    play, ///< 再生中.
    pause, ///< 一時停止中.
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
* スケルタル・メッシュ描画用UBOデータ.
*/
struct alignas(256) UniformDataMeshMatrix
{
  glm::aligned_vec4 color;
  glm::aligned_mat3x4 matModel[4]; // it must transpose.
  glm::aligned_mat3x4 matNormal[4]; // w isn't ussing. no need to transpose.
  glm::aligned_mat3x4 matBones[256]; // it must transpose.
};

/**
* アニメーション用の中間データ.
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