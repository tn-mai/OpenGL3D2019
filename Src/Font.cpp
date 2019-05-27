/**
* @file Font.cpp
*/
#define  _CRT_SECURE_NO_WARNINGS
#include "Font.h"
#include <memory>
#include <iostream>
#include <stdio.h>

/**
* フォント描画機能を格納する名前空間.
*/
namespace Font {

/**
* フォント描画オブジェクトを初期化する.
*
* @param maxChar   最大描画文字数.
*
* @retval true  初期化成功.
* @retval false 初期化失敗.
*/
bool Renderer::Init(size_t maxChar)
{
  return spriteRenderer.Init(maxChar, "Res/Font.vert", "Res/Font.frag");
}

/**
* フォントファイルを読み込む.
*
* @param filename フォントファイル名.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool Renderer::LoadFromFile(const char* filename)
{
  const std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(filename, "r"), &fclose);
  if (!fp) {
    return false;
  }

  int line = 1;
  int ret = fscanf(fp.get(), "info face=\"%*[^\"]\" size=%f bold=%*d italic=%*d charset=%*s"
    " unicode=%*d stretchH=%*d smooth=%*d aa=%*d padding=%f,%f,%f,%f spacing=%f,%f%*[^\n]",
    &baseFontSize, &padding[0], &padding[1], &padding[2], &padding[3],
    &spacing[0], &spacing[1]);
  if (ret < 7) {
    std::cerr << "ERROR: " << filename << "の読み込みに失敗(line=" << line << ")\n";
    return false;
  }
  ++line;

  glm::vec2 scale;
  ret = fscanf(fp.get(), " common lineHeight=%f base=%f scaleW=%f scaleH=%f pages=%*d packed=%*d%*[^\n]",
    &lineHeight, &base, &scale.x, &scale.y);
  if (ret < 4) {
    std::cerr << "ERROR: " << filename << "の読み込みに失敗(line=" << line << ")\n";
    return false;
  }
  ++line;

  // 位置情報ファイルのパス部分を取り出す.
  std::string basePath = filename;
  const size_t lastSlashIndex = basePath.find_last_of('/', std::string::npos);
  if (lastSlashIndex == std::string::npos) {
    basePath.clear();
  } else {
    basePath.resize(lastSlashIndex + 1);
  }

  std::vector<std::string> texNameList;
  for (;;) {
    char tex[256];
    ret = fscanf(fp.get(), " page id=%*d file=\"%255[^\"]\"", tex);
    if (ret < 1) {
      break;
    }
    tex[255] = '\0';
    texNameList.push_back(basePath + tex);
    ++line;
  }
  if (texNameList.empty()) {
    std::cerr << "ERROR: " << filename << "の読み込みに失敗(line=" << line << ")\n";
    return false;
  }

  int charCount;
  ret = fscanf(fp.get(), " chars count=%d", &charCount);
  if (ret < 1) {
    std::cerr << "ERROR: " << filename << "の読み込みに失敗(line=" << line << ")\n";
    return false;
  }
  ++line;

  fixedAdvance = 0;
  fontList.clear();
  fontList.resize(65536);
  for (int i = 0; i < charCount; ++i) {
    FontInfo font;
    ret = fscanf(fp.get(), " char id=%d x=%f y=%f width=%f height=%f xoffset=%f yoffset=%f xadvance=%f page=%d chnl=%*d",
      &font.id, &font.uv.x, &font.uv.y, &font.size.x, &font.size.y, &font.offset.x, &font.offset.y, &font.xadvance, &font.page);
    if (ret < 9) {
      std::cerr << "ERROR: " << filename << "の読み込みに失敗(line=" << line << ")\n";
      return false;
    }
    // フォントファイルは左上が原点なので、OpenGLの座標系(左下が原点)に変換.
    font.uv.y = scale.y - font.uv.y - font.size.y;
    if (font.id >= 0 && font.id < static_cast<int>(fontList.size())) {
      fontList[font.id] = font;
      if (font.xadvance > fixedAdvance) {
        fixedAdvance = font.xadvance;
      }
    }
    ++line;
  }

  // テクスチャを読み込む.
  texList.clear();
  texList.reserve(texNameList.size());
  for (const auto& e : texNameList) {
    Texture::Image2DPtr tex = Texture::Image2D::Create(e.c_str());
    if (!tex) {
      return false;
    }
    texList.push_back(tex);
  }
  return true;
}

/**
* 文字色を設定する.
*
* @param c文字色.
*/
void Renderer::Color(const glm::vec4& c)
{
  color = c;
}

/**
*文字色を取得する.
*
* @return文字色.
*/
const glm::vec4& Renderer::Color() const
{
  return color;
}

/**
* 文字列を追加する.
*
* @param position 表示開始座標.
* @param str      追加する文字列.
*
* @retval true  追加成功.
* @retval false 追加失敗.
*/
bool Renderer::AddString(const glm::vec2& position, const wchar_t* str)
{
  glm::vec2 pos = position;
  for (const wchar_t* itr = str; *itr; ++itr) {
    const FontInfo& font = fontList[*itr];
    if (font.id >= 0 && font.size.x && font.size.y) {
      // スプライトの座標は画像の中心を指定するが、フォントは左上を指定するので、その差を打ち消すための補正値を計算する.
      const float baseX = font.size.x * 0.5f + font.offset.x;
      const float baseY = base - font.size.y * 0.5f - font.offset.y;
      glm::vec3 offsetedPos = glm::vec3(pos + glm::vec2(baseX, baseY) * scale, 0);
      if (!propotional) {
        offsetedPos.x = pos.x;
      }
      Sprite sprite(texList[font.page]);
      sprite.Position(offsetedPos);
      sprite.Rectangle({ font.uv, font.size });
      sprite.Scale(scale);
      sprite.Color(color);
      if (!spriteRenderer.AddVertices(sprite)) {
        return false;
      }
    }
    pos.x += (propotional ? font.xadvance : fixedAdvance) * scale.x;
  }
  return true;
}

/**
* VBOをシステムメモリにマッピングする.
*/
void Renderer::BeginUpdate()
{
  spriteRenderer.BeginUpdate();
}

/**
* VBOのマッピングを解除する.
*/
void Renderer::EndUpdate()
{
  spriteRenderer.EndUpdate();
}

/**
* フォントを描画する.
*/
void Renderer::Draw(const glm::vec2& ss) const
{
  spriteRenderer.Draw(ss);
}

} // namespace Font
