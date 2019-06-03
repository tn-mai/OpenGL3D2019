/**
* @file Shader.cpp
*/
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <fstream>

namespace Shader {

/**
* �V�F�[�_�R�[�h���R���p�C������.
*
* @param type   �V�F�[�_�̎��.
* @param string �V�F�[�_�R�[�h�ւ̃|�C���^.
*
* @return �쐬�����V�F�[�_�I�u�W�F�N�g.
*/
GLuint Compile(GLenum type, const GLchar* string)
{
  if (!string) {
    return 0;
  }

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &string, nullptr);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
      std::vector<char> buf;
      buf.resize(infoLen);
      if (static_cast<int>(buf.size()) >= infoLen) {
        glGetShaderInfoLog(shader, infoLen, NULL, buf.data());
        std::cerr << "ERROR: �V�F�[�_�̃R���p�C���Ɏ��s\n" << buf.data() << std::endl;
      }
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

/**
* �v���O�����I�u�W�F�N�g���쐬����.
*
* @param vsCode ���_�V�F�[�_�R�[�h�ւ̃|�C���^.
* @param fsCode �t���O�����g�V�F�[�_�R�[�h�ւ̃|�C���^.
*
* @return �쐬�����v���O�����I�u�W�F�N�g.
*/
GLuint Build(const GLchar* vsCode, const GLchar* fsCode)
{
  GLuint vs = Compile(GL_VERTEX_SHADER, vsCode);
  GLuint fs = Compile(GL_FRAGMENT_SHADER, fsCode);
  if (!vs || !fs) {
    return 0;
  }
  GLuint program = glCreateProgram();
  glAttachShader(program, fs);
  glDeleteShader(fs);
  glAttachShader(program, vs);
  glDeleteShader(vs);
  glLinkProgram(program);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    GLint infoLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
      std::vector<char> buf;
      buf.resize(infoLen);
      if (static_cast<int>(buf.size()) >= infoLen) {
        glGetProgramInfoLog(program, infoLen, NULL, buf.data());
        std::cerr << "ERROR: �V�F�[�_�̃����N�Ɏ��s\n" << buf.data() << std::endl;
      }
    }
    glDeleteProgram(program);
    return 0;
  }
  return program;
}

/**
* �t�@�C����ǂݍ���.
*
* @param path �ǂݍ��ރt�@�C����.
*
* @return �ǂݍ��񂾃f�[�^.
*/
std::vector<GLchar> ReadFile(const char* path)
{
  std::basic_ifstream<GLchar> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs.is_open()) {
    std::cerr << "ERROR: " << path << " ���J���܂���\n";
    return {};
  }
  ifs.seekg(0, std::ios_base::end);
  const std::streamoff length = ifs.tellg();
  ifs.seekg(0, std::ios_base::beg);
  std::vector<GLchar> buf((size_t)length);
  ifs.read(buf.data(), length);
  buf.push_back('\0');
  return buf;
}

/**
* �t�@�C������v���O�����E�I�u�W�F�N�g���쐬����.
*
* @param vsPath ���_�V�F�[�_�[�t�@�C����.
* @param fsPath �t���O�����g�V�F�[�_�[�t�@�C����.
*
* @return �쐬�����v���O�����E�I�u�W�F�N�g.
*/
GLuint BuildFromFile(const char* vsPath, const char* fsPath)
{
  const std::vector<GLchar> vsCode = ReadFile(vsPath);
  const std::vector<GLchar> fsCode = ReadFile(fsPath);
  return Build(vsCode.data(), fsCode.data());
}

/**
* �R���X�g���N�^.
*
* @param vsPath ���_�V�F�[�_�[�t�@�C����.
* @param fsPath �t���O�����g�V�F�[�_�[�t�@�C����.
*/
Program::Program(const char* vsPath, const char* fsPath)
{
  id = BuildFromFile(vsPath, fsPath);
  if (id) {
    locMatVP = glGetUniformLocation(id, "matVP");
    locMatModel = glGetUniformLocation(id, "matModel");
  }
}

/**
* �f�X�g���N�^.
*
* �v���O�����E�I�u�W�F�N�g���폜����.
*/
Program::~Program()
{
  glDeleteProgram(id);
}

/**
* �v���O�����E�I�u�W�F�N�g���ݒ肳��Ă��邩���ׂ�.
*
* @retval true  �ݒ肳��Ă���.
* @retval false �ݒ肳��Ă��Ȃ�.
*/
bool Program::IsNull() const
{
  return id;
}

/**
* �v���O�����E�I�u�W�F�N�g���O���t�B�b�N�X�E�p�C�v���C���Ɋ��蓖�Ă�.
*
* �v���O�����E�I�u�W�F�N�g���g���I�������glUseProgram(0)�����s���ĉ������邱��.
*/
void Program::Use() const
{
  glUseProgram(id);
}

void Program::Unuse() const
{
  glUseProgram(0);
}

/**
* �`��Ɏg����r���[�E�v���W�F�N�V�����s���ݒ肷��.
*
* @param m �ݒ肷��r���[�E�v���W�F�N�V�����s��.
*
*/
void Program::SetViewProjectionMatrix(const glm::mat4& m) const
{
  if (locMatVP >= 0) {
    glUniformMatrix4fv(locMatVP, 1, GL_FALSE, &m[0][0]);
  }
}

/**
* �`��Ɏg���郂�f���s���ݒ肷��.
*
* @param m �ݒ肷�郂�f���s��.
*/
void Program::SetModelMatrix(const glm::mat4& m) const
{
  if (locMatModel >= 0) {
    glUniformMatrix4fv(locMatModel, 1, GL_FALSE, &m[0][0]);
  }
}

/**
* Uniform�u���b�N���o�C���f�B���O�E�|�C���g�Ɋ��蓖�Ă�.
*
* @param blockName    ���蓖�Ă�Uniform�u���b�N�̖��O.
* @param bindingPoint ���蓖�Đ�̃o�C���f�B���O�E�|�C���g.
*
* @retval true  ���蓖�Đ���.
* @retval false ���蓖�Ď��s.
*/
bool Program::BindUniformBlock(const char* blockName, GLuint bindingPoint)
{
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
* ���j�t�H�[���ϐ��̈ʒu���擾����.
*
* @param uniformName ���j�t�H�[���ϐ��̖��O.
*
* @retval 0      ���O��uniformName�ƈ�v����ϐ���������Ȃ�. �܂��̓V�F�[�_�[������.
* @retval 1�ȏ�  ���O��uniformName�ƈ�v����ϐ��̈ʒu.
*/
GLint Program::GetUniformLocation(const char* uniformName) const
{
  if (id) {
    return glGetUniformLocation(id, uniformName);
  }
  return 0;
}

/**
* ���j�t�H�[���ϐ��Ƀf�[�^��ݒ肷��.
*/
void Program::SetUniformInt(GLint location, GLint value) const
{
  if (id) {
    return glUniform1i(location, value);
  }
}

/**
* �V���O���g���C���X�^���X���擾����.
*
* @return Cache�̃V���O���g���C���X�^���X.
*/
Cache& Cache::Instance()
{
  static Cache instance;
  return instance;
}

/**
* �R���X�g���N�^.
*/
Cache::Cache()
{
  map.reserve(256);
}

/**
* �V�F�[�_�[�v���O�������쐬���A�L���b�V������.
*
* @param vsPath ���_�V�F�[�_�[�t�@�C����.
* @param fsPath �t���O�����g�V�F�[�_�[�t�@�C����.
*
* @return vsPath��fsPath����쐬���ꂽ�V�F�[�_�[�v���O����.
*/
ProgramPtr Cache::Create(const char* vsPath, const char* fsPath)
{
  const std::string key = std::string(vsPath) + std::string(fsPath);
  const auto itr = map.find(key);
  if (itr != map.end()) {
    return itr->second;
  }
  const ProgramPtr value = std::make_shared<Program>(vsPath, fsPath);
  map.insert(std::make_pair(key, value));
  return value;
}

/**
* �V�F�[�_�[�v���O�������擾����.
*
* @param vsPath ���_�V�F�[�_�[�t�@�C����.
* @param fsPath �t���O�����g�V�F�[�_�[�t�@�C����.
*
* @return vsPath��fsPath����쐬���ꂽ�V�F�[�_�[�v���O����.
*         �Ή�����G���g�����Ȃ��ꍇ��nullptr��Ԃ�.
*/
ProgramPtr Cache::Get(const char* vsPath, const char* fsPath) const
{
  const std::string key = std::string(vsPath) + std::string(fsPath);
  const auto itr = map.find(key);
  if (itr != map.end()) {
    return itr->second;
  }
  return ProgramPtr();
}

/**
* �ǂݍ��񂾑S�ẴV�F�[�_�[�v���O������j������.
*/
void Cache::Clear()
{
  map.clear();
}

} // namespace Shader
