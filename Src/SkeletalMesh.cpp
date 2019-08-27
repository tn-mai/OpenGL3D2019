/**
* @file SleletalMesh.cpp
*/
#define NOMINMAX
#include "SkeletalMesh.h"
#include "UniformBuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

namespace Mesh {

/**
*
*/
namespace GlobalSkeletalMeshState {

namespace /* unnamed */ {

/**
* スケルタル・メッシュ描画用UBOデータ.
*/
struct alignas(256) UniformDataMeshMatrix
{
  glm::aligned_vec4 color;
  glm::aligned_mat3x4 matModel[8]; // it must transpose.
  glm::aligned_mat3x4 matBones[256]; // it must transpose.
};

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
    int hasTransformation = 0;
  };
  std::vector<Transformation> nodeTransformations;
};

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
    glm::aligned_mat4 m(1);
    if (transformation.hasTransformation & 1) {
      const glm::aligned_mat4 T = glm::translate(glm::aligned_mat4(1), transformation.translation);
      m = T;
    }
    if (transformation.hasTransformation & 2) {
      const glm::aligned_mat4 R = glm::mat4_cast(transformation.rotation);
      m = m * R;
    }
    if (transformation.hasTransformation & 4) {
      const glm::aligned_mat4 S = glm::scale(glm::aligned_mat4(1), transformation.scale);
      m = m * S;
    }
    transformation.matLocal = transformation.matLocal * m;
  } else {
    transformation.matLocal *= node.matLocal;
  }
  transformation.matGlobal = transformation.matLocal * node.matInverseBindPose;
  transformation.isCalculated = true;
}

/**
* アニメーション補間された座標変換行列を計算する.
*/
AnimatedNodeTree MakeAnimatedNodeTree(const ExtendedFile& file, const Animation& animation, float keyFrame)
{
  AnimatedNodeTree tmp;
  tmp.nodeTransformations.resize(file.nodes.size());
  for (const auto& e : animation.scaleList) {
    tmp.nodeTransformations[e.targetNodeId].scale = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation |= 4;
  }
  for (const auto& e : animation.rotationList) {
    tmp.nodeTransformations[e.targetNodeId].rotation = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation |= 2;
  }
  for (const auto& e : animation.translationList) {
    tmp.nodeTransformations[e.targetNodeId].translation = Interporation(e, keyFrame);
    tmp.nodeTransformations[e.targetNodeId].hasTransformation |= 1;
  }
  for (auto& e : file.nodes) {
    CalcGlobalTransform(file.nodes, e, tmp);
  }
  return tmp;
}

// Implemented in Mesh.cpp
std::vector<char> ReadFromFile(const char* path);
void GetBuffer(const json11::Json& accessor, const json11::Json& bufferViews, const std::vector<std::vector<char>>& binFiles, const void** pp, size_t* pLength, int* pStride = nullptr);

/**
*
*/
struct MeshTransformation {
  std::vector<glm::aligned_mat4> transformations;
  std::vector<glm::aligned_mat4> matRoot;
};

/**
* アニメーションを適用した座標変換行列リストを計算する.
*
* @param file      アニメーションとノードを所有するファイルオブジェクト.
* @param node      スキニング対象のノード.
* @param animation 計算の元になるアニメーション.
* @param frame     アニメーションの再生位置.
*
* @return アニメーションを適用した座標変換行列リスト.
*/
MeshTransformation CalculateTransform(const ExtendedFilePtr& file, const Node* node, const Animation* animation, float frame)
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
* コンストラクタ.
*
* @param p 親となるメッシュバッファ.
* @param f メッシュファイル.
* @param n メッシュノード.
*/
SkeletalMesh::SkeletalMesh(const ExtendedFilePtr& f, const Node* n)
  : file(f), node(n)
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
  // 再生フレーム更新.
  if (animation && state == State::play) {
    frame += deltaTime * animationSpeed;
    if (loop) {
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
  GlobalSkeletalMeshState::UniformDataMeshMatrix uboData;
  uboData.color = color;
  const MeshTransformation mt = CalculateTransform(file, node, animation, frame);
  for (size_t i = 0; i < mt.transformations.size(); ++i) {
    uboData.matBones[i] = glm::transpose(mt.transformations[i]);
  }
  if (mt.matRoot.empty()) {
    uboData.matModel[0] = glm::transpose(matModel);
  } else {
    const size_t size = std::min(mt.matRoot.size(), sizeof(uboData.matModel) / sizeof(uboData.matModel[0]));
    for (size_t i = 0; i < size; ++i) {
      const glm::aligned_mat4 m = matModel * mt.matRoot[i];
      uboData.matModel[i] = glm::transpose(m);
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
      if (!loop && (frame >= animation->totalTime)) {
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
      const GLint locMaterialColor = glGetUniformLocation(m.progSkeletalMesh->Id(), "materialColor");
      if (locMaterialColor >= 0) {
        glUniform4fv(locMaterialColor, 1, &m.baseColor.x);
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
* @param loop      ループ再生の指定(true=ループする false=ループしない).
*
* @retval true  再生開始.
* @retval false 再生失敗.
*/
bool SkeletalMesh::Play(const std::string& animationName, bool loop)
{
  if (file) {
    for (const auto& e : file->animations) {
      if (e.name == animationName) {
        animation = &e;
        frame = 0;
        state = State::play;
        this->loop = loop;
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
* @param position 再生位置(秒).
*/
void SkeletalMesh::SetPosition(float position)
{
  frame = position;
  if (animation) {
    if (loop) {
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

/**
* アニメーションの再生位置を取得する.
*
* @return 再生位置(秒).
*/
float SkeletalMesh::GetPosition() const
{
  return frame;
}

/**
* アニメーションの再生が終了しているか調べる.
*
* @retval true  終了している.
* @retval false 終了していない. または、一度も有効な名前でPlay()が実行されていない.
*
* ループ再生中の場合、この関数は常にfalseを返すことに注意.
*/
bool SkeletalMesh::IsFinished() const
{
  if (!file || !animation) {
    return false;
  }
  return animation->totalTime <= frame;
}

/**
* ループ再生の有無を取得する.
*
* @retval true ループ再生される.
* @retval false ループ再生されない.
*/
bool SkeletalMesh::Loop() const
{
  return loop;
}

/**
* ループ再生の有無を取得する.
*
* @param loop ループ再生の有無.
*/
void SkeletalMesh::Loop(bool loop)
{
  this->loop = loop;
}

namespace /* unnamed */ {

/**
* JSONの配列データをglm::vec3に変換する.
*
* @param json 変換元となる配列データ.
*
* @return jsonを変換してできたvec3の値.
*/
glm::vec3 GetVec3(const json11::Json& json)
{
  const std::vector<json11::Json>& a = json.array_items();
  if (a.size() < 3) {
    return glm::vec3(0);
  }
  return glm::vec3(a[0].number_value(), a[1].number_value(), a[2].number_value());
}

/**
* JSONの配列データをglm::quatに変換する.
*
* @param json 変換元となる配列データ.
*
* @return jsonを変換してできたquatの値.
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
* JSONの配列データをglm::mat4に変換する.
*
* @param json 変換元となる配列データ.
*
* @return jsonを変換してできたmat4の値.
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
* ノードのローカル姿勢行列を計算する.
*
* @param node gltfノード.
*
* @return nodeのローカル姿勢行列.
*/
glm::mat4 CalcLocalMatrix(const json11::Json& node)
{
  if (node["matrix"].is_array()) {
    return GetMat4(node["matrix"]);
  } else {
    glm::mat4 m(1);
    if (node["translation"].is_array()) {
      m *= glm::translate(glm::mat4(1), GetVec3(node["translation"]));
    }
    if (node["rotation"].is_array()) {
      m *= glm::mat4_cast(GetQuat(node["rotation"]));
    }
    if (node["scale"].is_array()) {
      m *= glm::scale(glm::mat4(1), GetVec3(node["scale"]));
    }
    return m;
  }
}

} // unnamed namespace

/**
* glTFファイルを読み込む.
*
* @param path glTFファイル名.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool Buffer::LoadSkeletalMesh(const char* path)
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

  ExtendedFilePtr pFile = std::make_shared<ExtendedFile>();
  ExtendedFile& file = *pFile;

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
      const int accessorId_weights = attributes["WEIGHTS_0"].is_null() ? -1 : attributes["WEIGHTS_0"].int_value();
      const int accessorId_joints = attributes["JOINTS_0"].is_null() ? -1 : attributes["JOINTS_0"].int_value();

      mesh.primitives[primId].vao = std::make_shared<VertexArrayObject>();
      mesh.primitives[primId].vao->Create(vbo.Id(), ibo.Id());
      SetAttribute(mesh.primitives[primId], 0, accessors[accessorId_position], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 2, accessors[accessorId_texcoord], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 3, accessors[accessorId_normal], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 4, accessors[accessorId_weights], bufferViews, binFiles);
      SetAttribute(mesh.primitives[primId], 5, accessors[accessorId_joints], bufferViews, binFiles);

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

  // ノードツリーを構築.
  {
    const json11::Json& nodes = json["nodes"];
    int i = 0;
    file.nodes.resize(nodes.array_items().size());
    for (const auto& node : nodes.array_items()) {
      // 親子関係を構築.
      // NOTE: ポインタを使わずともインデックスで十分かもしれない.
      const std::vector<json11::Json>& children = node["children"].array_items();
      file.nodes[i].children.reserve(children.size());
      for (const auto& e : children) {
        const int childJointId = e.int_value();
        file.nodes[i].children.push_back(&file.nodes[childJointId]);
        if (!file.nodes[childJointId].parent) {
          file.nodes[childJointId].parent = &file.nodes[i];
        }
      }

      // ローカル座標変換行列を計算.
      file.nodes[i].matLocal = CalcLocalMatrix(nodes[i]);

      ++i;
    }

    // シーンのルートノードを取得.
    file.scenes.reserve(json["scenes"].array_items().size());
    for (const auto& scene : json["scenes"].array_items()) {
      Scene tmp;
      tmp.rootNode = scene.int_value();
      GetMeshNodeList(&file.nodes[tmp.rootNode], tmp.meshNodes);
      file.scenes.push_back(tmp);
    }
  }

  {
    for (size_t i = 0; i < file.nodes.size(); ++i) {
      file.nodes[i].matGlobal = file.nodes[i].matLocal;
      Node* parent = file.nodes[i].parent;
      while (parent) {
        file.nodes[i].matGlobal = parent->matLocal * file.nodes[i].matGlobal;
        parent = parent->parent;
      }
    }
  }

  file.skins.reserve(json["skins"].array_items().size());
  for (const auto& skin : json["skins"].array_items()) {
    Skin tmpSkin;

    // バインドポーズ行列を取得.
    const json11::Json& accessor = accessors[skin["inverseBindMatrices"].int_value()];
    if (accessor["type"].string_value() != "MAT4") {
      std::cerr << "ERROR: バインドポーズのtypeはMAT4でなくてはなりません \n";
      std::cerr << "  type = " << accessor["type"].string_value() << "\n";
      return false;
    }
    if (accessor["componentType"].int_value() != GL_FLOAT) {
      std::cerr << "ERROR: バインドポーズのcomponentTypeはGL_FLOATでなくてはなりません \n";
      std::cerr << "  type = 0x" << std::hex << accessor["componentType"].string_value() << "\n";
      return false;
    }

    const void* p;
    size_t byteLength;
    GetBuffer(accessor, bufferViews, binFiles, &p, &byteLength);

    // gltfのバッファデータはリトルエンディアン. 仕様に書いてある.
    const std::vector<json11::Json>& joints = skin["joints"].array_items();
    std::vector<glm::mat4> inverseBindPoseList;
    inverseBindPoseList.resize(accessor["count"].int_value());
    memcpy(inverseBindPoseList.data(), p, std::min(byteLength, inverseBindPoseList.size() * 64));
    tmpSkin.joints.resize(joints.size());
    for (size_t i = 0; i < joints.size(); ++i) {
      const int jointId = joints[i].int_value();
      tmpSkin.joints[i] = jointId;
      file.nodes[jointId].matInverseBindPose = inverseBindPoseList[i];
    }
    tmpSkin.name = skin["name"].string_value();
    file.skins.push_back(tmpSkin);
  }

  {
    const std::vector<json11::Json>& nodes = json["nodes"].array_items();
    for (size_t i = 0; i < nodes.size(); ++i) {
      const json11::Json& meshId = nodes[i]["mesh"];
      if (meshId.is_number()) {
        file.nodes[i].mesh = meshId.int_value();
      }
      const json11::Json& skinId = nodes[i]["skin"];
      if (skinId.is_number()) {
        file.nodes[i].skin = skinId.int_value();
      }
    }
  }

  // アニメーション.
  {
    for (const auto& animation : json["animations"].array_items()) {
      Animation anime;
      anime.translationList.reserve(32);
      anime.rotationList.reserve(32);
      anime.scaleList.reserve(32);
      anime.name = animation["name"].string_value();

      const std::vector<json11::Json>& channels = animation["channels"].array_items();
      const std::vector<json11::Json>& samplers = animation["samplers"].array_items();
      for (const json11::Json& e : channels) {
        const int samplerId = e["sampler"].int_value();
        const json11::Json& sampler = samplers[samplerId];
        const json11::Json& target = e["target"];
        const int targetNodeId = target["node"].int_value();
        if (targetNodeId < 0) {
          continue;
        }
        const int inputAccessorId = sampler["input"].int_value();
        const int inputCount = accessors[inputAccessorId]["count"].int_value();
        const void* pInput;
        size_t inputByteLength;
        GetBuffer(accessors[inputAccessorId], bufferViews, binFiles, &pInput, &inputByteLength);

        const int outputAccessorId = sampler["output"].int_value();
        const int outputCount = accessors[outputAccessorId]["count"].int_value();
        const void* pOutput;
        size_t outputByteLength;
        GetBuffer(accessors[outputAccessorId], bufferViews, binFiles, &pOutput, &outputByteLength);

        const std::string& path = target["path"].string_value();
        anime.totalTime = 0;
        if (path == "translation") {
          const GLfloat* pKeyFrame = static_cast<const GLfloat*>(pInput);
          const glm::vec3* pData = static_cast<const glm::vec3*>(pOutput);
          Timeline<glm::aligned_vec3> timeline;
          timeline.timeline.reserve(inputCount);
          for (int i = 0; i < inputCount; ++i) {
            anime.totalTime = std::max(anime.totalTime, pKeyFrame[i]);
            timeline.timeline.push_back({ pKeyFrame[i], pData[i] });
          }
          timeline.targetNodeId = targetNodeId;
          anime.translationList.push_back(timeline);
        } else if (path == "rotation") {
          const GLfloat* pKeyFrame = static_cast<const GLfloat*>(pInput);
          const glm::quat* pData = static_cast<const glm::quat*>(pOutput);
          Timeline<glm::aligned_quat> timeline;
          timeline.timeline.reserve(inputCount);
          for (int i = 0; i < inputCount; ++i) {
            anime.totalTime = std::max(anime.totalTime, pKeyFrame[i]);
            timeline.timeline.push_back({ pKeyFrame[i], pData[i] });
          }
          timeline.targetNodeId = targetNodeId;
          anime.rotationList.push_back(timeline);
        } else if (path == "scale") {
          const GLfloat* pKeyFrame = static_cast<const GLfloat*>(pInput);
          const glm::vec3* pData = static_cast<const glm::vec3*>(pOutput);
          Timeline<glm::aligned_vec3> timeline;
          timeline.timeline.reserve(inputCount);
          for (int i = 0; i < inputCount; ++i) {
            anime.totalTime = std::max(anime.totalTime, pKeyFrame[i]);
            timeline.timeline.push_back({ pKeyFrame[i], pData[i] });
          }
          timeline.targetNodeId = targetNodeId;
          anime.scaleList.push_back(timeline);
        }
      }
      file.animations.push_back(anime);
    }
  }

  file.name = path;
  extendedFiles.insert(std::make_pair(file.name, pFile));
  for (size_t i = 0; i < file.nodes.size(); ++i) {
    const int meshIndex = file.nodes[i].mesh;
    if (meshIndex < 0) {
      continue;
    }
    const MeshData& mesh = file.meshes[meshIndex];
    meshes.insert(std::make_pair(mesh.name, MeshIndex{ pFile, &pFile->nodes[i] }));
  }

  std::cout << "[INFO]" << __func__ << ": '" << path << "'を読み込みました.\n";
  std::cout << "  total nodes = " << file.nodes.size() << "\n";
  for (size_t i = 0; i < file.meshes.size(); ++i) {
    std::cout << "  mesh[" << i << "] = " << file.meshes[i].name << "\n";
  }
  for (size_t i = 0; i < file.animations.size(); ++i) {
    std::cout << "  animation[" << i << "] = " << file.animations[i].name << "(" << file.animations[i].totalTime << "sec)\n";
  }
  for (size_t i = 0; i < file.skins.size(); ++i) {
    std::cout << "  skin[" << i << "] = " << file.skins[i].name << "(" << file.skins[i].joints.size() << ")\n";
  }

  return true;
}

/**
* スケルタルメッシュを取得する.
*
* @param meshName 取得したいスケルタルメッシュの名前.
*
* @return meshNameと同じ名前を持つスケルタルメッシュ.
*/
SkeletalMeshPtr Buffer::GetSkeletalMesh(const char* meshName) const
{
  const auto itr = meshes.find(meshName);
  if (itr == meshes.end()) {
    static SkeletalMeshPtr dummy(std::make_shared<SkeletalMesh>());
    return dummy;
  }

  ExtendedFilePtr pFile = itr->second.file;
  const Node* pNode = itr->second.node;
  return std::make_shared<SkeletalMesh>(pFile, pNode);
}

} // namespace Mesh