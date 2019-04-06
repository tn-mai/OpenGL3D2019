/**
* @file BufferObject.cpp
*/
#include "BufferObject.h"
#include <iostream>

/**
* バッファオブジェクトを作成する.
*
* @param target バッファオブジェクトの種類.
* @param size   頂点データのサイズ.
* @param data   頂点データへのポインタ.
* @param usage  バッファオブジェクトのアクセスタイプ.
*/
bool BufferObject::Create(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
  Destroy();
  glGenBuffers(1, &id);
  glBindBuffer(target, id);
  glBufferData(target, size, data, usage);
  glBindBuffer(target, 0);
  this->target = target;
  this->size = size;
  return glGetError() == GL_NO_ERROR;
}

/**
* Buffer Objectを破棄する.
*/
bool BufferObject::BufferSubData(GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
  if (offset + size >= this->size) {
    std::cerr << "WARNING: 転送先領域がバッファサイズを越えています.\n";
    std::cerr << "  buffer size:" << this->size << " offset:" << offset << " size:" << size << "\n";
    if (offset >= this->size) {
      return false;
    }
    // 可能な範囲だけ転送を行う.
    size = this->size - offset;
  }
  glBindBuffer(target, id);
  glBufferSubData(target, offset, size, data);
  glBindBuffer(target, 0);
  return glGetError() == GL_NO_ERROR;
}

/**
* Buffer Objectを破棄する.
*/
void BufferObject::Destroy()
{
  if (id) {
    glDeleteBuffers(1, &id);
    id = 0;
  }
}

/**
* VAOを作成する.
*
* @param vbo  頂点バッファオブジェクトのID.
* @param ibo  インデックスバッファオブジェクトのID.
*/
bool VertexArrayObject::Create(GLuint vbo, GLuint ibo)
{
  Destroy();
  glGenVertexArrays(1, &id);
  glBindVertexArray(id);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  vboId = vbo;
  iboId = ibo;
  return glGetError() == GL_NO_ERROR;
}

/**
* VAOを破棄する.
*/
void VertexArrayObject::Destroy()
{
  if (id) {
    glDeleteVertexArrays(1, &id);
    id = 0;
    vboId = 0;
  }
}

/**
* VAOをOpenGLコンテキストにバインドする.
*
* 描画、VertexAttribPointer()、ResetVertexAttribPointer()を呼び出す前に、この関数を実行してVAOをバインドする必要がある.
*
* @sa Unbind(), VertexAttribPointer(), ResetVertexAttribPointer()
*/
void VertexArrayObject::Bind() const
{
	glBindVertexArray(id);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
}

/**
* OpenGLコンテキストへのバインドを解除する.
*
* 描画、VertexAttribPointer()、ResetVertexAttribPointer()が終わったら、この関数を実行してバインドを解除しなければならない.
*
* @sa Bind(), VertexAttribPointer(), ResetVertexAttribPointer()
*/
void VertexArrayObject::Unbind() const
{
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
* 頂点アトリビュートを設定する.
*
* @param index      頂点アトリビュートのインデックス.
* @param size       頂点アトリビュートの要素数.
* @param type       頂点アトリビュートの型.
* @param normalized GL_TRUE=要素を正規化する. GL_FALSE=正規化しない.
* @param stride     次の頂点データまでのバイト数.
* @param offset     頂点データ先頭からのバイトオフセット.
*
* Bind()してからUnbind()するまでの間で呼び出すこと.
*
* @sa Bind(), Unbind(), ResetVertexAttribPointer()
*/
void VertexArrayObject::VertexAttribPointer(
  GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, size_t offset) const
{
  glEnableVertexAttribArray(index);
  glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<GLvoid*>(offset));
}

/**
* 全ての頂点アトリビュートを無効化する.
*
* @sa Bind(), Unbind(), VertexAttribPointer()
*/
void VertexArrayObject::ResetVertexAttribPointer() const
{
  GLint maxAttr;
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttr);
  for (int i = 0; i < maxAttr; ++i) {
    glDisableVertexAttribArray(i);
  }
}
