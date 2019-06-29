/**
* @file Texture.h
*/
#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED
#include <GL/glew.h>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>
#include <string>

namespace Texture {

class Image2D;
using Image2DPtr = std::shared_ptr<Image2D>;

/**
* 画像データ.
*/
class ImageData {
public:
  glm::vec4 GetColor(int x, int y) const;

public:
  std::string name; ///< ファイル名.
  GLint width = 0; ///< 横のピクセル数.
  GLint height = 0; ///< 縦のピクセル数.
  GLenum format = GL_NONE; ///< どの色情報が記録されているか.
  GLenum type = GL_NONE; ///< それぞれの色情報のビット配置.
  std::vector<uint8_t> data; ///< バイトデータ.
};

GLuint CreateImage2D(GLsizei width, GLsizei height, const GLvoid* data, GLenum format, GLenum type);
GLuint LoadImage2D(const char* path);
bool LoadImage2D(const char* path, ImageData& imageData);

/**
* テクスチャ・イメージ.
*/
class Image2D
{
public:
  static Image2DPtr Create(const char*);

  ~Image2D();
  bool IsNull() const;
  const std::string& Name() const { return name; }
  GLint Width() const { return width; }
  GLint Height() const { return height; }
  GLuint Id() const { return id; }
  GLenum Target() const;
  void Bind(int no) const;
  void Unbind(int no) const;

private:
  Image2D() = default;
  explicit Image2D(const char*);

  std::string name; ///< テクスチャファイル名.
  GLuint id = 0;
  GLint width = 0;
  GLint height = 0;
  bool isCubemap = false;
};

} // namespace Texture

#endif // TEXTURE_H_INCLUDED