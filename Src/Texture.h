/**
* @file Texture.h
*/
#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED
#include <GL/glew.h>

namespace Texture {

GLuint CreateImage2D(GLsizei width, GLsizei height, const GLvoid* data, GLenum format, GLenum type);
GLuint LoadImage2D(const char* path);

/**
* テクスチャ・イメージ.
*/
class Image2D
{
public:
  Image2D() = default;
  explicit Image2D(const char*);
  ~Image2D();
  bool IsNull() const;
  GLuint Id() const { return id; }
  GLenum Target() const;
  void Bind(int no) const;
  void Unbind(int no) const;

private:
  GLuint id = 0;
  GLint width = 0;
  GLint height = 0;
  bool isCubemap = false;
};

} // namespace Texture

#endif // TEXTURE_H_INCLUDED