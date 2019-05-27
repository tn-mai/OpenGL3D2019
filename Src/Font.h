/**
* @file Font.h
*/
#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED
#include "Sprite.h"
#include <vector>
#include <string>

namespace Font {

/**
* ビットマップフォント描画クラス.
*/
class Renderer
{
public:
  Renderer() = default;
  ~Renderer() = default;
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool Init(size_t maxChar);
  bool LoadFromFile(const char* filename);

  void Scale(const glm::vec2& s) { scale = s; }
  const glm::vec2& Scale() const { return scale; }
  void Color(const glm::vec4& c);
  const glm::vec4& Color() const;
  float LineHeight() const { return lineHeight; }

  void BeginUpdate();
  bool AddString(const glm::vec2& position, const wchar_t* str);
  void EndUpdate();
  void Draw(const glm::vec2& ss) const;

private:
  SpriteRenderer spriteRenderer;
  std::vector<Texture::Image2DPtr> texList;

  glm::vec2 scale = glm::vec2(1); ///< フォントを描画するときの拡大率.
  glm::vec4 color = glm::vec4(1); ///< フォントを描画するときの色.
  float baseFontSize = 32;
  bool propotional = true;
  float fixedAdvance = 0;
  float padding[4] = { 0, 0, 0, 0 };
  float spacing[2] = { 0, 0 };
  float lineHeight = 0;
  float base = 0;

  /// フォント情報.
  struct FontInfo {
    int id = -1;        ///< 文字コード.
    int page = 0;       ///< フォントが含まれる画像の番号.
    glm::vec2 uv = glm::vec2(0); ///< フォント画像のテクスチャ座標.
    glm::vec2 size = glm::vec2(0);     ///< フォント画像の表示サイズ.
    glm::vec2 offset = glm::vec2(0);   ///< 表示位置をずらす距離.
    float xadvance = 0; ///< カーソルを進める距離.
  };
  std::vector<FontInfo> fontList; ///< フォント位置情報のリスト.
};

} // namespace Font

#endif // FONT_H_INCLUDED
