/**
* @file Shader.h
*/
#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED
#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <memory>

namespace Shader {

GLuint Build(const GLchar* vsCode, const GLchar* fsCode);
GLuint BuildFromFile(const char* vsPath, const char* fsPath);

/**
* シェーダー・プログラム.
*/
class Program
{
public:
  Program() = delete;
  Program(const char* vsPath, const char* fsPath);
  ~Program();

  bool IsNull() const;
  void Use() const;
  void Unuse() const;
  void SetViewProjectionMatrix(const glm::mat4&) const;

private:
  GLuint id = 0; // プログラムID.
  GLint locMatVP = -1;
};
using ProgramPtr = std::shared_ptr<Program>;

/**
* シェーダープログラムをキャッシュするクラス.
*/
class Cache
{
public:
  static Cache& Instance();

  ProgramPtr Create(const char* vsPath, const char* fsPath);
  void Clear();

private:
  Cache();
  ~Cache() = default;
  Cache(const Cache&) = delete;
  Cache& operator=(const Cache&) = delete;

  std::unordered_map<std::string, std::shared_ptr<Program>> map;
};

} // namespace Shader

#endif // SHADER_H_INCLUDED
