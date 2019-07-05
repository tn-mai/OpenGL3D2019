/**
* @file SleletalMesh.cpp
*/
#include "SkeletalMesh.h"
#include "UniformBuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace Mesh {

namespace GlobalSkeletalMeshState {

namespace /* unnamed */ {

const char UniformNameForBoneMatrix[] = "MeshMatrixUniformData";

bool isInitialized = false;
int currentUboIndex = 0;
UniformBufferPtr ubo[2];
std::vector<uint8_t> uboData;

} // unnamed namespace

/**
*
*/
bool Initialize()
{
  if (!isInitialized) {
    const size_t maxMeshCount = 1024;
    const GLsizeiptr uboSize = static_cast<GLsizeiptr>(sizeof(UniformDataMeshMatrix) * maxMeshCount);
    ubo[0] = UniformBuffer::Create(uboSize, 0, UniformNameForBoneMatrix);
    ubo[1] = UniformBuffer::Create(uboSize, 0, UniformNameForBoneMatrix);
    uboData.reserve(uboSize);
    currentUboIndex = 0;

    isInitialized = true;
  }

  return true;
}

/**
*
*/
void Finalize()
{
  if (isInitialized) {
    uboData.clear();
    uboData.shrink_to_fit();
    ubo[1].reset();
    ubo[0].reset();
    isInitialized = false;
  }
}

/**
*
*/
bool BindUniformBlock(const Shader::ProgramPtr& program)
{
  static const GLuint bindingPoint = 0;
  static const char* const blockName = UniformNameForBoneMatrix;

  const GLuint id = program->Id();
  const GLuint blockIndex = glGetUniformBlockIndex(id, blockName);
  if (blockIndex == GL_INVALID_INDEX) {
    std::cerr << "[エラー] Uniformブロック'" << blockName << "'が見つかりません\n";
    return false;
  }
  glUniformBlockBinding(id, blockIndex, bindingPoint);
  const GLenum result = glGetError();
  if (result != GL_NO_ERROR) {
    std::cerr << "[エラー] Uniformブロック'" << blockName << "'のバインドに失敗\n";
    return false;
  }
  return true;
}

/**
*
*/
void ResetUniformData()
{
  if (!isInitialized) {
    return;
  }

  uboData.clear();
}

/**
*
*/
GLintptr PushUniformData(const void* data, size_t size)
{
  if (!isInitialized) {
    return -1;
  }

  UniformBufferPtr pUbo = ubo[currentUboIndex];
  if (uboData.size() + size >= static_cast<size_t>(pUbo->Size())) {
    return -1;
  }
  const uint8_t* p = static_cast<const uint8_t*>(data);
  const GLintptr offset = static_cast<GLintptr>(uboData.size());
  uboData.insert(uboData.end(), p, p + size);
  uboData.resize(((uboData.size() + 255) / 256) * 256);
  return offset;
}

/**
*
*/
void UploadUniformData()
{
  if (!isInitialized) {
    return;
  }

  if (!uboData.empty()) {
    UniformBufferPtr pUbo = ubo[currentUboIndex];
    pUbo->BufferSubData(uboData.data(), 0, uboData.size());
    currentUboIndex = !currentUboIndex;
  }
}

/**
*
*/
void BindUniformData(GLintptr offset, GLsizeiptr size)
{
  if (!isInitialized) {
    return;
  }

  ubo[!currentUboIndex]->BindBufferRange(offset, size);
}

} // namespace GlobalSkeletalMeshState

/**
* 座標変換行列から回転行列を取り出す.
*
* @param m 座標変換行列.
*
* @return mの回転要素だけからなる回転行列.
*/
glm::aligned_mat4 DecomposeRotation(const glm::aligned_mat4& m)
{
  glm::aligned_vec3 scale;
  scale.x = 1.0f / glm::length(glm::aligned_vec3(m[0]));
  scale.y = 1.0f / glm::length(glm::aligned_vec3(m[1]));
  scale.z = 1.0f / glm::length(glm::aligned_vec3(m[2]));

  return glm::aligned_mat3(glm::scale(m, scale));
}

/**
*
*/
void GetMeshNodeList(const Node* node, std::vector<const Node*>& list)
{
  if (node->mesh >= 0) {
    list.push_back(node);
  }
  for (const auto& child : node->children) {
    GetMeshNodeList(child, list);
  }
}

/**
*
*/
void CalcGlobalTransform(const std::vector<Node>& nodes, const Node& node, AnimatedNodeTree& animated)
{
  const int currentNodeId = &node - &nodes[0];
  AnimatedNodeTree::Transformation& transformation = animated.nodeTransformations[currentNodeId];
  if (transformation.isCalculated) {
    return;
  }

  if (node.parent) {
    CalcGlobalTransform(nodes, *node.parent, animated);
    const int parentNodeId = node.parent - &nodes[0];
    transformation.matLocal = animated.nodeTransformations[parentNodeId].matLocal;
  } else {
    transformation.matLocal = glm::aligned_mat4(1);
  }
  if (transformation.hasTransformation) {
    const glm::aligned_mat4 T = glm::translate(glm::aligned_mat4(1), transformation.translation);
    const glm::aligned_mat4 R = glm::mat4_cast(transformation.rotation);
    const glm::aligned_mat4 S = glm::scale(glm::aligned_mat4(1), transformation.scale);
    transformation.matLocal *= T * R * S;
  } else {
    transformation.matLocal *= node.matLocal;
  }
  transformation.matGlobal = transformation.matLocal * node.matInverseBindPose;
  transformation.isCalculated = true;
}

/**
* アニメーション補間された座標変換行列を計算する.
*/
AnimatedNodeTree MakeAnimatedNodeTree(const File& file, const Animation& animation, float keyFrame)
{
  AnimatedNodeTree tmp;
  tmp.nodeTransformations.resize(file.nodes.size());
  for (const auto& e : animation.scaleList) {
    tmp.nodeTransformations[e.targetNodeId].scale = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation = true;
  }
  for (const auto& e : animation.rotationList) {
    tmp.nodeTransformations[e.targetNodeId].rotation = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation = true;
  }
  for (const auto& e : animation.translationList) {
    tmp.nodeTransformations[e.targetNodeId].translation = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation = true;
  }
  for (auto& e : file.nodes) {
    CalcGlobalTransform(file.nodes, e, tmp);
  }
  return tmp;
}

/**
* コンストラクタ.
*
* @param p 親となるメッシュバッファ.
* @param f メッシュファイル.
* @param n メッシュノード.
*/
SkeletalMesh::SkeletalMesh(Buffer* p, const FilePtr& f, const Node* n)
  : parent(p), file(f), node(n)
{
}

/**
* アニメーション状態を更新する.
*
* @param deltaTime 前回の更新からの経過時間.
* @param matModel  モデル行列.
* @param color     メッシュの色.
*/
void SkeletalMesh::Update(float deltaTime, const glm::aligned_mat4& matModel, const glm::vec4& color)
{
  if (!parent) {
    return;
  }

  // 再生フレーム更新.
  if (animation && state == State::play) {
    frame += deltaTime * animationSpeed;
    if (doesLoop) {
      if (frame >= animation->totalTime) {
        frame -= animation->totalTime;
      } else if (frame < 0) {
        const float n = std::ceil(-frame / animation->totalTime);
        frame += animation->totalTime * n;
      }
    } else {
      if (frame >= animation->totalTime) {
        frame = animation->totalTime;
      } else if (frame < 0) {
        frame = 0;
      }
    }
  }

  // 親BufferにUBOデータを追加.
  UniformDataMeshMatrix uboData;
  uboData.color = color;
  const MeshTransformation mt = CalculateTransform(file, node, animation, frame);
  for (size_t i = 0; i < mt.transformations.size(); ++i) {
    uboData.matBones[i] = glm::transpose(mt.transformations[i]);
  }
  if (mt.matRoot.empty()) {
    uboData.matModel[0] = glm::transpose(matModel);
    uboData.matNormal[0] = DecomposeRotation(matModel);
  } else {
    for (size_t i = 0; i < mt.matRoot.size(); ++i) {
      const glm::aligned_mat4 m = matModel * mt.matRoot[i];
      uboData.matModel[i] = glm::transpose(m);
      uboData.matNormal[i] = DecomposeRotation(m);
    }
  }
  uboSize = sizeof(glm::aligned_vec4) + sizeof(glm::aligned_mat3x4) * 8 + sizeof(glm::aligned_mat3x4) * mt.transformations.size();
  uboSize = ((uboSize + 255) / 256) * 256;
  uboOffset = GlobalSkeletalMeshState::PushUniformData(&uboData, uboSize);

  // 状態を更新.
  if (animation) {
    switch (state) {
    case State::stop:
      break;
    case State::play:
      if (!doesLoop && animation->totalTime >= frame) {
        state = State::stop;
      }
      break;
    case State::pause:
      break;
    }
  }
}

/**
* スケルタルメッシュを描画する.
*/
void SkeletalMesh::Draw() const
{
  if (!file) {
    return;
  }

  // TODO: シーンレベルの描画に対応すること.
  //std::vector<const Node*> meshNodes;
  //meshNodes.reserve(32);
  //GetMeshNodeList(node, meshNodes);

  GlobalSkeletalMeshState::BindUniformData(uboOffset, uboSize);

  const MeshData& meshData = file->meshes[node->mesh];
  GLuint prevTexId = 0;
  for (const auto& prim : meshData.primitives) {
    prim.vao->Bind();
    if (!prim.hasColorAttribute) {
      static const glm::vec4 color(1);
      glVertexAttrib4fv(1, &color.x);
    }

    if (prim.material >= 0 && prim.material < static_cast<int>(file->materials.size())) {
      const Material& m = file->materials[prim.material];
      m.progSkeletalMesh->Use();
      if (m.texture) {
        const GLuint texId = m.texture->Id();
        if (prevTexId != texId) {
          m.texture->Bind(0);
          prevTexId = texId;
        }
      }
      glDrawElementsBaseVertex(prim.mode, prim.count, prim.type, prim.indices, prim.baseVertex);
    }
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
* アニメーションの再生状態を取得する.
*
* @return 再生状態を示すState列挙型の値.
*/
SkeletalMesh::State SkeletalMesh::GetState() const
{
  return state;
}

/**
* メッシュに関連付けられたアニメーションのリストを取得する.
*
* @return アニメーションリスト.
*/
const std::vector<Animation>& SkeletalMesh::GetAnimationList() const
{
  if (!file) {
    static const std::vector<Animation> dummy;
    return dummy;
  }
  return file->animations;
}

/**
* アニメーション時間を取得する.
*
* @return アニメーション時間(秒).
*/
float SkeletalMesh::GetTotalAnimationTime() const
{
  if (!file || !animation) {
    return 0;
  }
  return animation->totalTime;
}

/**
* アニメーションを再生する.
*
* @param animationName 再生するアニメーションの名前.
* @param doesLoop      ループ再生の指定(true=ループする false=ループしない).
*
* @retval true  再生開始.
* @retval false 再生失敗.
*/
bool SkeletalMesh::Play(const std::string& animationName, bool doesLoop)
{
  if (file) {
    for (const auto& e : file->animations) {
      if (e.name == animationName) {
        animation = &e;
        frame = 0;
        state = State::play;
        this->doesLoop = doesLoop;
        return true;
      }
    }
  }
  return false;
}

/**
* アニメーションの再生を停止する.
*
* @retval true  成功.
* @retval false 失敗(アニメーションが設定されていない).
*/
bool SkeletalMesh::Stop()
{
  if (animation) {
    switch (state) {
    case State::play:
      state = State::stop;
      return true;
    case State::stop:
      return true;
    case State::pause:
      state = State::stop;
      return true;
    }
  }
  return false;
}

/**
* アニメーションの再生を一時停止する.
*
* @retval true  成功.
* @retval false 失敗(アニメーションが停止している、またはアニメーションが設定されていない).
*/
bool SkeletalMesh::Pause()
{
  if (animation) {
    switch (state) {
    case State::play:
      state = State::pause;
      return true;
    case State::stop:
      return false;
    case State::pause:
      return true;
    }
  }
  return false;
}

/**
* アニメーションの再生を再開する.
*
* @retval true  成功.
* @retval false 失敗(アニメーションが停止している、またはアニメーションが設定されていない).
*/
bool SkeletalMesh::Resume()
{
  if (animation) {
    switch (state) {
    case State::play:
      return true;
    case State::stop:
      return false;
    case State::pause:
      state = State::play;
      return true;
    }
  }
  return false;
}

/**
* 再生中のアニメーション名を取得する.
*
* @return 再生中のアニメーション名.
*         一度もPlay()に成功していない場合、空の文字列が返される.
*/
const std::string& SkeletalMesh::GetAnimation() const
{
  if (!animation) {
    static const std::string dummy("");
    return dummy;
  }
  return animation->name;
}

/**
* アニメーションの再生速度を設定する.
*
* @param speed 再生速度(1.0f=等速, 2.0f=2倍速, 0.5f=1/2倍速).
*/
void SkeletalMesh::SetAnimationSpeed(float speed)
{
  animationSpeed = speed;
}

/**
* アニメーションの再生速度を取得する.
*
* @return 再生速度.
*/
float SkeletalMesh::GetAnimationSpeed() const
{
  return animationSpeed;
}

/**
* アニメーションの再生位置を設定する.
*
* @param 
*/
void SkeletalMesh::SetPosition(float pos)
{
  frame = pos;
  if (animation) {
    if (doesLoop) {
      if (frame >= animation->totalTime) {
        frame -= animation->totalTime;
      } else if (frame < 0) {
        const float n = std::ceil(-frame / animation->totalTime);
        frame += animation->totalTime * n;
      }
    } else {
      if (frame >= animation->totalTime) {
        frame = animation->totalTime;
      } else if (frame < 0) {
        frame = 0;
      }
    }
  }
}

float SkeletalMesh::GetPosition() const
{
  return frame;
}

/**
*
*/
MeshTransformation CalculateTransform(const FilePtr& file, const Node* node, const Animation* animation, float frame)
{
  MeshTransformation transformation;
  if (file && node) {
    std::vector<const Node*> meshNodes;
    meshNodes.reserve(32);
    GetMeshNodeList(node, meshNodes);

    if (animation) {
      const AnimatedNodeTree tmp = MakeAnimatedNodeTree(*file, *animation, frame);
      if (node->skin >= 0) {
        const std::vector<int>& joints = file->skins[node->skin].joints;
        transformation.transformations.resize(joints.size());
        for (size_t i = 0; i < joints.size(); ++i) {
          const int jointNodeId = joints[i];
          transformation.transformations[i] = tmp.nodeTransformations[jointNodeId].matGlobal;
        }
        transformation.matRoot.resize(meshNodes.size(), glm::mat4(1));
      } else {
        transformation.matRoot.reserve(meshNodes.size());
        for (const auto& e : meshNodes) {
          const size_t nodeId = e - &file->nodes[0];
          transformation.matRoot.push_back(tmp.nodeTransformations[nodeId].matGlobal);
        }
      }
    }

    if (node->skin >= 0) {
      if (animation) {
        const AnimatedNodeTree tmp = MakeAnimatedNodeTree(*file, *animation, frame);
        const std::vector<int>& joints = file->skins[node->skin].joints;
        transformation.transformations.resize(joints.size());
        for (size_t i = 0; i < joints.size(); ++i) {
          const int jointNodeId = joints[i];
          transformation.transformations[i] = tmp.nodeTransformations[jointNodeId].matGlobal;
        }
      } else {
        const std::vector<int>& joints = file->skins[node->skin].joints;
        transformation.transformations.resize(joints.size(), glm::mat4(1));
      }
      transformation.matRoot.resize(meshNodes.size(), glm::mat4(1));
    } else {
      transformation.matRoot.reserve(meshNodes.size());
      for (const auto& e : meshNodes) {
        transformation.matRoot.push_back(e->matGlobal);
      }
    }
  }
  return transformation;
}

} // namespace Mesh