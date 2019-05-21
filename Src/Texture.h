/**
* @file Texture.h
*/
#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED
#include <GL/glew.h>
#include <memory>
#include <vector>

namespace Texture {

class Image2D;
using Image2DPtr = std::shared_ptr<Image2D>;

struct ImageData {
  GLint width;
  GLint height;
  GLenum format;
  GLenum type;
  std::vector<uint8_t> data;
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
  GLint Width() const { return width; }
  GLint Height() const { return height; }
  GLuint Id() const { return id; }
  GLenum Target() const;
  void Bind(int no) const;
  void Unbind(int no) const;

private:
  Image2D() = default;
  explicit Image2D(const char*);

  GLuint id = 0;
  GLint width = 0;
  GLint height = 0;
  bool isCubemap = false;
};

} // namespace Texture

#endif // TEXTURE_H_INCLUDED