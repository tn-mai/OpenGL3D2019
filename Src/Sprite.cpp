/**
* @file Sprite.cpp
*/
#include "Sprite.h"
#include <vector>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

/**
* Spriteコンストラクタ.
*/
Sprite::Sprite(const Texture::Image2DPtr& tex) :
  texture(tex),
  rect(Rect{ glm::vec2(), glm::vec2(tex->Width(), tex->Height()) })
{
}

/**
* 描画に使用するテクスチャを指定する.
*
* @param tex 描画に使用するテクスチャ.
*/
void Sprite::Texture(const Texture::Image2DPtr& tex)
{
  texture = tex;
  Rectangle(Rect{ glm::vec2(0), glm::vec2(tex->Width(), tex->Height()) });
}

/**
* スプライト描画クラスを初期化する.
*
* @param maxSpriteCount 描画可能な最大スプライト数.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool SpriteRenderer::Init(size_t maxSpriteCount, const char* vsPath, const char* fsPath)
{
  vbo.Create(GL_ARRAY_BUFFER, sizeof(Vertex) * maxSpriteCount * 4, nullptr, GL_STREAM_DRAW);
  std::vector<GLushort> indices;
  indices.resize(maxSpriteCount * 6);
  for (GLushort i = 0; i < maxSpriteCount; ++i) {
    indices[i * 6 + 0] = (i * 4) + 0;
    indices[i * 6 + 1] = (i * 4) + 1;
    indices[i * 6 + 2] = (i * 4) + 2;
    indices[i * 6 + 3] = (i * 4) + 2;
    indices[i * 6 + 4] = (i * 4) + 3;
    indices[i * 6 + 5] = (i * 4) + 0;
  }
  ibo.Create(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), indices.data());
  vao.Create(vbo.Id(), ibo.Id());
  vao.Bind();
  vao.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, position));
  vao.VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, color));
  vao.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoord));
  vao.Unbind();
  program = Shader::Cache::Instance().Create(vsPath, fsPath);

  drawDataList.reserve(128);

  if (!vbo.Id() || !ibo.Id() || !vao.Id() || program->IsNull()) {
    return false;
  }
  return true;
}

/**
* 頂点データの作成を開始する.
*/
void SpriteRenderer::BeginUpdate()
{
  drawDataList.clear();
  vertices.clear();
  vertices.reserve(vbo.Size() / sizeof(Vertex));
}

/**
* 頂点データを追加する.
*
* @param sprite 頂点データの元になるスプライト.
*
* @retval true  追加成功.
* @retval false 頂点バッファが満杯で追加できない.
*/
bool SpriteRenderer::AddVertices(const Sprite& sprite)
{
  if (vertices.size() * sizeof(Vertex) >= static_cast<size_t>(vbo.Size())) {
    std::cerr << "[警告] " << __func__ << "VBOが満杯です\n";
    return false;
  }
  const Texture::Image2DPtr& texture = sprite.Texture();
  const glm::vec2 textureSize(texture->Width(), texture->Height());
  const glm::vec2 reciprocalSize(glm::vec2(1) / textureSize);

  Rect rect = sprite.Rectangle();
  rect.origin *= reciprocalSize;
  rect.size *= reciprocalSize;
  const glm::vec2 halfSize = sprite.Rectangle().size * 0.5f;
  const glm::mat4 matT = glm::translate(glm::mat4(1), sprite.Position());
  const glm::mat4 matR = glm::rotate(glm::mat4(1), sprite.Rotation(), glm::vec3(0, 0, 1));
  const glm::mat4 matS = glm::scale(glm::mat4(1), glm::vec3(sprite.Scale(), 1));
  const glm::mat4 transform = matT * matR * matS;

  Vertex v[4];

  v[0].position = transform * glm::vec4(-halfSize.x, -halfSize.y, 0, 1);
  v[0].color = sprite.Color();
  v[0].texCoord = rect.origin;

  v[1].position = transform * glm::vec4(halfSize.x, -halfSize.y, 0, 1);
  v[1].color = sprite.Color();
  v[1].texCoord = glm::vec2(rect.origin.x + rect.size.x, rect.origin.y);

  v[2].position = transform * glm::vec4(halfSize.x, halfSize.y, 0, 1);
  v[2].color = sprite.Color();
  v[2].texCoord = rect.origin + rect.size;

  v[3].position = transform * glm::vec4(-halfSize.x, halfSize.y, 0, 1);
  v[3].color = sprite.Color();
  v[3].texCoord = glm::vec2(rect.origin.x, rect.origin.y + rect.size.y);

  vertices.insert(vertices.end(), v, v + 4);

  if (drawDataList.empty()) {
    drawDataList.push_back({ 6, 0, texture });
  } else {
    auto& data = drawDataList.back();
    if (data.texture == texture) {
      data.count += 6;
    } else {
      drawDataList.push_back({ 6, data.offset + data.count * sizeof(GLushort), texture });
    }
  }
  
  return true;
}

/**
* 頂点データの作成を終了する.
*/
void SpriteRenderer::EndUpdate()
{
  vbo.BufferSubData(0, vertices.size() * sizeof(Vertex), vertices.data());
  vertices.clear();
  vertices.shrink_to_fit();
}

/**
* スプライトを描画する.
*
* @param texture    描画に使用するテクスチャ.
* @param screenSize 画面サイズ.
*/
void SpriteRenderer::Draw(const glm::vec2& screenSize) const
{
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  vao.Bind();
  program->Use();

  // 平行投影、原点は画面の中心.
  const glm::vec2 halfScreenSize = screenSize * 0.5f;
  const glm::mat4x4 matProj = glm::ortho(-halfScreenSize.x, halfScreenSize.x, -halfScreenSize.y, halfScreenSize.y, 1.0f, 1000.0f);
  const glm::mat4x4 matView = glm::lookAt(glm::vec3(0, 0, 100), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  program->SetViewProjectionMatrix(matProj * matView);

  for (const auto& data : drawDataList) {
    data.texture->Bind(0);
    glDrawElements(GL_TRIANGLES, data.count, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(data.offset));
    data.texture->Unbind(0);
  }

  program->Unuse();
  vao.Unbind();
}

/**
* スプライト描画データを消去する.
*/
void SpriteRenderer::ClearDrawData()
{
  drawDataList.clear();
}