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
    std::cerr << "[�G���[] Uniform�u���b�N'" << blockName << "'��������܂���\n";
    return false;
  }
  glUniformBlockBinding(id, blockIndex, bindingPoint);
  const GLenum result = glGetError();
  if (result != GL_NO_ERROR) {
    std::cerr << "[�G���[] Uniform�u���b�N'" << blockName << "'�̃o�C���h�Ɏ��s\n";
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
* ���W�ϊ��s�񂩂��]�s������o��.
*
* @param m ���W�ϊ��s��.
*
* @return m�̉�]�v�f��������Ȃ��]�s��.
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
* �A�j���[�V������Ԃ��ꂽ���W�ϊ��s����v�Z����.
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
* �R���X�g���N�^.
*
* @param p �e�ƂȂ郁�b�V���o�b�t�@.
* @param f ���b�V���t�@�C��.
* @param n ���b�V���m�[�h.
*/
SkeletalMesh::SkeletalMesh(Buffer* p, const FilePtr& f, const Node* n)
  : parent(p), file(f), node(n)
{
}

/**
* �A�j���[�V������Ԃ��X�V����.
*
* @param deltaTime �O��̍X�V����̌o�ߎ���.
* @param matModel  ���f���s��.
* @param color     ���b�V���̐F.
*/
void SkeletalMesh::Update(float deltaTime, const glm::aligned_mat4& matModel, const glm::vec4& color)
{
  if (!parent) {
    return;
  }

  // �Đ��t���[���X�V.
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

  // �eBuffer��UBO�f�[�^��ǉ�.
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

  // ��Ԃ��X�V.
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
* �X�P���^�����b�V����`�悷��.
*/
void SkeletalMesh::Draw() const
{
  if (!file) {
    return;
  }

  // TODO: �V�[�����x���̕`��ɑΉ����邱��.
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
* �A�j���[�V�����̍Đ���Ԃ��擾����.
*
* @return �Đ���Ԃ�����State�񋓌^�̒l.
*/
SkeletalMesh::State SkeletalMesh::GetState() const
{
  return state;
}

/**
* ���b�V���Ɋ֘A�t����ꂽ�A�j���[�V�����̃��X�g���擾����.
*
* @return �A�j���[�V�������X�g.
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
* �A�j���[�V�������Ԃ��擾����.
*
* @return �A�j���[�V��������(�b).
*/
float SkeletalMesh::GetTotalAnimationTime() const
{
  if (!file || !animation) {
    return 0;
  }
  return animation->totalTime;
}

/**
* �A�j���[�V�������Đ�����.
*
* @param animationName �Đ�����A�j���[�V�����̖��O.
* @param doesLoop      ���[�v�Đ��̎w��(true=���[�v���� false=���[�v���Ȃ�).
*
* @retval true  �Đ��J�n.
* @retval false �Đ����s.
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
* �A�j���[�V�����̍Đ����~����.
*
* @retval true  ����.
* @retval false ���s(�A�j���[�V�������ݒ肳��Ă��Ȃ�).
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
* �A�j���[�V�����̍Đ����ꎞ��~����.
*
* @retval true  ����.
* @retval false ���s(�A�j���[�V��������~���Ă���A�܂��̓A�j���[�V�������ݒ肳��Ă��Ȃ�).
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
* �A�j���[�V�����̍Đ����ĊJ����.
*
* @retval true  ����.
* @retval false ���s(�A�j���[�V��������~���Ă���A�܂��̓A�j���[�V�������ݒ肳��Ă��Ȃ�).
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
* �Đ����̃A�j���[�V���������擾����.
*
* @return �Đ����̃A�j���[�V������.
*         ��x��Play()�ɐ������Ă��Ȃ��ꍇ�A��̕����񂪕Ԃ����.
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
* �A�j���[�V�����̍Đ����x��ݒ肷��.
*
* @param speed �Đ����x(1.0f=����, 2.0f=2�{��, 0.5f=1/2�{��).
*/
void SkeletalMesh::SetAnimationSpeed(float speed)
{
  animationSpeed = speed;
}

/**
* �A�j���[�V�����̍Đ����x���擾����.
*
* @return �Đ����x.
*/
float SkeletalMesh::GetAnimationSpeed() const
{
  return animationSpeed;
}

/**
* �A�j���[�V�����̍Đ��ʒu��ݒ肷��.
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