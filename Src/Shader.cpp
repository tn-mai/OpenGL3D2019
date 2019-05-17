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
* シェーダコードをコンパイルする.
*
* @param type   シェーダの種類.
* @param string シェーダコードへのポインタ.
*
* @return 作成したシェーダオブジェクト.
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
        std::cerr << "ERROR: シェーダのコンパイルに失敗\n" << buf.data() << std::endl;
      }
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

/**
* プログラムオブジェクトを作成する.
*
* @param vsCode 頂点シェーダコードへのポインタ.
* @param fsCode フラグメントシェーダコードへのポインタ.
*
* @return 作成したプログラムオブジェクト.
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
        std::cerr << "ERROR: シェーダのリンクに失敗\n" << buf.data() << std::endl;
      }
    }
    glDeleteProgram(program);
    return 0;
  }
  return program;
}

/**
* ファイルを読み込む.
*
* @param path 読み込むファイル名.
*
* @return 読み込んだデータ.
*/
std::vector<GLchar> ReadFile(const char* path)
{
  std::basic_ifstream<GLchar> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs.is_open()) {
    std::cerr << "ERROR: " << path << " を開けません\n";
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
* ファイルからプログラム・オブジェクトを作成する.
*
* @param vsPath 頂点シェーダーファイル名.
* @param fsPath フラグメントシェーダーファイル名.
*
* @return 作成したプログラム・オブジェクト.
*/
GLuint BuildFromFile(const char* vsPath, const char* fsPath)
{
  const std::vector<GLchar> vsCode = ReadFile(vsPath);
  const std::vector<GLchar> fsCode = ReadFile(fsPath);
  return Build(vsCode.data(), fsCode.data());
}

/**
* コンストラクタ.
*
* @param vsPath 頂点シェーダーファイル名.
* @param fsPath フラグメントシェーダーファイル名.
*/
Program::Program(const char* vsPath, const char* fsPath)
{
  id = BuildFromFile(vsPath, fsPath);
  if (id) {
    locMatVP = glGetUniformLocation(id, "matVP");
  }
}

/**
* デストラクタ.
*
* プログラム・オブジェクトを削除する.
*/
Program::~Program()
{
  glDeleteProgram(id);
}

/**
* プログラム・オブジェクトが設定されているか調べる.
*
* @retval true  設定されている.
* @retval false 設定されていない.
*/
bool Program::IsNull() const
{
  return id;
}

/**
* プログラム・オブジェクトをグラフィックス・パイプラインに割り当てる.
*
* プログラム・オブジェクトを使い終わったらglUseProgram(0)を実行して解除すること.
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
* 描画に使われるビュー・プロジェクション行列を設定する.
*
* @param m 設定するビュー・プロジェクション行列.
*
*/
void Program::SetViewProjectionMatrix(const glm::mat4& m) const
{
  if (locMatVP >= 0) {
    glUniformMatrix4fv(locMatVP, 1, GL_FALSE, &m[0][0]);
  }
}

/**
* Uniformブロックをバインディング・ポイントに割り当てる.
*
* @param blockName    割り当てるUniformブロックの名前.
* @param bindingPoint 割り当て先のバインディング・ポイント.
*
* @retval true  割り当て成功.
* @retval false 割り当て失敗.
*/
bool Program::BindUniformBlock(const char* blockName, GLuint bindingPoint)
{
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
* ユニフォーム変数の位置を取得する.
*
* @param uniformName ユニフォーム変数の名前.
*
* @retval 0      名前がuniformNameと一致する変数が見つからない. またはシェーダーが無効.
* @retval 1以上  名前がuniformNameと一致する変数の位置.
*/
GLint Program::GetUniformLocation(const char* uniformName) const
{
  if (id) {
    return glGetUniformLocation(id, uniformName);
  }
  return 0;
}

/**
* ユニフォーム変数にデータを設定する.
*/
void Program::SetUniformInt(GLint location, GLint value) const
{
  if (id) {
    return glUniform1i(location, value);
  }
}

/**
* シングルトンインスタンスを取得する.
*
* @return Cacheのシングルトンインスタンス.
*/
Cache& Cache::Instance()
{
  static Cache instance;
  return instance;
}

/**
* コンストラクタ.
*/
Cache::Cache()
{
  map.reserve(256);
}

/**
* シェーダープログラムを作成し、キャッシュする.
*
* @param vsPath 頂点シェーダーファイル名.
* @param fsPath フラグメントシェーダーファイル名.
*
* @return vsPathとfsPathから作成されたシェーダープログラム.
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
* シェーダープログラムを取得する.
*
* @param vsPath 頂点シェーダーファイル名.
* @param fsPath フラグメントシェーダーファイル名.
*
* @return vsPathとfsPathから作成されたシェーダープログラム.
*         対応するエントリがない場合はnullptrを返す.
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
* 読み込んだ全てのシェーダープログラムを破棄する.
*/
void Cache::Clear()
{
  map.clear();
}

} // namespace Shader
