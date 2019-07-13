/**
* @file SleletalMesh.h
*/
#ifndef SKELETAL_MESH_H_INCLUDED
#define SKELETAL_MESH_H_INCLUDED
#include "Mesh.h"

namespace Mesh {

// スキンデータ.
struct Skin {
  std::string name;
  std::vector<int> joints;
};

// ノード.
struct Node {
  Node* parent = nullptr;
  int mesh = -1;
  int skin = -1;
  std::vector<Node*> children;
  glm::aligned_mat4 matLocal = glm::aligned_mat4(1);
  glm::aligned_mat4 matGlobal = glm::aligned_mat4(1);
  glm::aligned_mat4 matInverseBindPose = glm::aligned_mat4(1);
};

// アニメーションのキーフレーム.
template<typename T>
struct KeyFrame {
  float frame;
  T value;
};

// アニメーションのタイムライン.
template<typename T>
struct Timeline {
  int targetNodeId;
  std::vector<KeyFrame<T>> timeline;
};

// アニメーション.
struct Animation {
  std::vector<Timeline<glm::aligned_vec3>> translationList;
  std::vector<Timeline<glm::aligned_quat>> rotationList;
  std::vector<Timeline<glm::aligned_vec3>> scaleList;
  float totalTime = 0;
  std::string name;
};

// シーン.
struct Scene {
  int rootNode;
  std::vector<const Node*> meshNodes;
};

// 拡張ファイル.
struct ExtendedFile {
  std::string name; // ファイル名.
  std::vector<MeshData> meshes;
  std::vector<Material> materials;

  std::vector<Scene> scenes;
  std::vector<Node> nodes;
  std::vector<Skin> skins;
  std::vector<Animation> animations;
};

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
* 5. Mesh::BufferクラスにLoadSkeletalMesh関数、GetSkeletalMesh関数を追加.
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

} // namespace GlobalSkeletalMeshState

} // namespace Mesh

#endif // SKELETAL_MESH_H_INCLUDED