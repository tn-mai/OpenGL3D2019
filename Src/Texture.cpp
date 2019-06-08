/**
* @file Texture.cpp
*/
#define NOMINMAX
#include "Texture.h"
#include "d3dx12.h"
#include <cstdint>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <Windows.h>

/// テクスチャ関連の関数やクラスを格納する名前空間.
namespace Texture {
/**
* FOURCCを作成する.
*/
#define MAKE_FOURCC(a, b, c, d) static_cast<uint32_t>(a + (b << 8) + (c << 16) + (d << 24))

/**
* バイト列から数値を復元する.
*
* @param p      バイト列へのポインタ.
* @param offset 数値のオフセット.
* @param size   数値のバイト数(1～4).
*
* @return 復元した数値.
*/
uint32_t Get(const uint8_t* p, size_t offset, size_t size)
{
  uint32_t n = 0;
  p += offset;
  for (size_t i = 0; i < size; ++i) {
    n += p[i] << (i * 8);
  }
  return n;
}

/**
* DDS画像情報.
*/
struct DDSPixelFormat
{
  uint32_t size; ///< この構造体のバイト数(32).
  uint32_t flgas; ///< 画像に含まれるデータの種類を示すフラグ.
  uint32_t fourCC; ///< 画像フォーマットを示すFOURCC.
  uint32_t rgbBitCount; ///< 1ピクセルのビット数.
  uint32_t redBitMask; ///< 赤要素が使う部分を示すビット.
  uint32_t greenBitMask; ///< 緑要素が使う部分を示すビット.
  uint32_t blueBitMask; ///< 青要素が使う部分を示すビット.
  uint32_t alphaBitMask; ///< 透明要素が使う部分を示すビット.
};

/**
* バッファからDDS画像情報を読み出す.
*
* @param buf 読み出し元バッファ.
*
* @return 読み出したDDS画像情報.
*/
DDSPixelFormat ReadDDSPixelFormat(const uint8_t* buf)
{
  DDSPixelFormat tmp;
  tmp.size = Get(buf, 0, 4);
  tmp.flgas = Get(buf, 4, 4);
  tmp.fourCC = Get(buf, 8, 4);
  tmp.rgbBitCount = Get(buf, 12, 4);
  tmp.redBitMask = Get(buf, 16, 4);
  tmp.greenBitMask = Get(buf, 20, 4);
  tmp.blueBitMask = Get(buf, 24, 4);
  tmp.alphaBitMask = Get(buf, 28, 4);
  return tmp;
}

/**
* DDSファイルヘッダ.
*/
struct DDSHeader
{
  uint32_t size;  ///< この構造体のバイト数(124).
  uint32_t flags; ///< どのパラメータが有効かを示すフラグ.
  uint32_t height; ///< 画像の高さ(ピクセル数).
  uint32_t width; ///< 画像の幅(ピクセル数).
  uint32_t pitchOrLinearSize; ///< 横のバイト数または画像1枚のバイト数.
  uint32_t depth; ///< 画像の奥行き(枚数)(3次元テクスチャ等で使用).
  uint32_t mipMapCount; ///< 含まれているミップマップレベル数.
  uint32_t reserved1[11]; ///< (将来のために予約されている).
  DDSPixelFormat ddspf; ///< DDS画像情報.
  uint32_t caps[4]; ///< 含まれている画像の種類.
  uint32_t reserved2; ///< (将来のために予約されている).
};

/**
* バッファからDDSファイルヘッダを読み出す.
*
* @param buf 読み出し元バッファ.
*
* @return 読み出したDDSファイルヘッダ.
*/
DDSHeader ReadDDSHeader(const uint8_t* buf)
{
  DDSHeader tmp = {};
  tmp.size = Get(buf, 0, 4);
  tmp.flags = Get(buf, 4, 4);
  tmp.height = Get(buf, 8, 4);
  tmp.width = Get(buf, 12, 4);
  tmp.pitchOrLinearSize = Get(buf, 16, 4);
  tmp.depth = Get(buf, 20, 4);
  tmp.mipMapCount = Get(buf, 24, 4);
  tmp.ddspf = ReadDDSPixelFormat(buf + 28 + 4 * 11);
  for (int i = 0; i < 4; ++i) {
    tmp.caps[i] = Get(buf, 28 + 4 * 11 + 32 + i * 4, 4);
  }
  return tmp;
}

/**
* DDSファイルヘッダ(DirectX10拡張).
*/
struct DDSHeaderDX10
{
  uint32_t dxgiFormat; ///< 画像の種類(DX10以降に追加された種類).
  uint32_t resourceDimension; ///< 次元数(1D or 2D or 3D).
  uint32_t miscFlag; ///< 画像が想定する使われ方を示すフラグ.
  uint32_t arraySize; ///< 格納されているのがテクスチャ配列の場合、その配列サイズ.
  uint32_t reserved; ///< (将来のために予約されている).
};

/**
* バッファからDDSファイルヘッダ(DirectX10拡張)を読み出す.
*
* @param buf 読み出し元バッファ.
*
* @return 読み出したDDSファイルヘッダ(DirectX10拡張).
*/
DDSHeaderDX10 ReadDDSHeaderDX10(std::basic_ifstream<uint8_t>& ifs)
{
  uint8_t buf[20];
  ifs.read(buf, sizeof(buf));

  DDSHeaderDX10 tmp;
  tmp.dxgiFormat = Get(buf, 0, 4);
  tmp.resourceDimension = Get(buf, 4, 4);
  tmp.miscFlag = Get(buf, 8, 4);
  tmp.arraySize = Get(buf, 12, 4);
  tmp.reserved = Get(buf, 16, 4);
  return tmp;
}

/**
* DDSファイルからテクスチャを作成する.
*
* @param filename DDSファイル名.
* @param st       DDSファイルステータス.
* @param buf      ファイルを読み込んだバッファ.
* @param header   DDSヘッダ格納先へのポインタ.
*
* @retval 0以外 作成したテクスチャID.
* @retval 0     作成失敗.
*/
GLuint LoadDDS(const char* filename, std::basic_ifstream<uint8_t>& ifs, size_t fileSize)
{
  if (fileSize < 128) {
    std::cerr << "WARNING: " << filename << "はDDSファイルではありません.\n";
    return 0;
  }
  uint8_t ddsHeader[128];
  ifs.read(ddsHeader, sizeof(ddsHeader));
  const DDSHeader header = ReadDDSHeader(ddsHeader + 4);
  if (header.size != 124) {
    std::cerr << "WARNING: " << filename << "はDDSファイルではありません.\n";
    return 0;
  }
  GLenum iformat;
  GLenum format = GL_RGBA;
  size_t imageOffset = 0;
  uint32_t blockSize = 16;
  bool isCompressed = false;
  if (header.ddspf.flgas & 0x04) {
    switch (header.ddspf.fourCC) {
    case MAKE_FOURCC('D', 'X', 'T', '1'):
      iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      blockSize = 8;
      break;
    case MAKE_FOURCC('D', 'X', 'T', '2'):
    case MAKE_FOURCC('D', 'X', 'T', '3'):
      iformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      break;
    case MAKE_FOURCC('D', 'X', 'T', '4'):
    case MAKE_FOURCC('D', 'X', 'T', '5'):
      iformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      break;
    case MAKE_FOURCC('B', 'C', '4', 'U'):
      iformat = GL_COMPRESSED_RED_RGTC1;
      break;
    case MAKE_FOURCC('B', 'C', '4', 'S'):
      iformat = GL_COMPRESSED_SIGNED_RED_RGTC1;
      break;
    case MAKE_FOURCC('B', 'C', '5', 'U'):
      iformat = GL_COMPRESSED_RG_RGTC2;
      break;
    case MAKE_FOURCC('B', 'C', '5', 'S'):
      iformat = GL_COMPRESSED_SIGNED_RG_RGTC2;
      break;
    case MAKE_FOURCC('D', 'X', '1', '0'):
    {
      const DDSHeaderDX10 headerDX10 = ReadDDSHeaderDX10(ifs);
      switch (headerDX10.dxgiFormat) {
      case DXGI_FORMAT_BC1_UNORM: iformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; blockSize = 8; break;
      case DXGI_FORMAT_BC2_UNORM: iformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
      case DXGI_FORMAT_BC3_UNORM: iformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
      case DXGI_FORMAT_BC1_UNORM_SRGB: iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT; blockSize = 8; break;
      case DXGI_FORMAT_BC2_UNORM_SRGB: iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT; break;
      case DXGI_FORMAT_BC3_UNORM_SRGB: iformat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT; break;
      case DXGI_FORMAT_BC4_UNORM: iformat = GL_COMPRESSED_RED_RGTC1; break;
      case DXGI_FORMAT_BC4_SNORM: iformat = GL_COMPRESSED_SIGNED_RED_RGTC1; break;
      case DXGI_FORMAT_BC5_UNORM: iformat = GL_COMPRESSED_RG_RGTC2; break;
      case DXGI_FORMAT_BC5_SNORM: iformat = GL_COMPRESSED_SIGNED_RG_RGTC2; break;
      case DXGI_FORMAT_BC6H_UF16: iformat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT; break;
      case DXGI_FORMAT_BC6H_SF16: iformat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT; break;
      case DXGI_FORMAT_BC7_UNORM: iformat = GL_COMPRESSED_RGBA_BPTC_UNORM; break;
      case DXGI_FORMAT_BC7_UNORM_SRGB: iformat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM; break;
      default:
        std::cerr << "WARNING: " << filename << "は未対応のDDSファイルです.\n";
        return 0;
      }
      imageOffset = 20; // DX10ヘッダのぶんを加算.
      break;
    }
    default:
      std::cerr << "WARNING: " << filename << "は未対応のDDSファイルです.\n";
      return 0;
    }
    isCompressed = true;
  } else if (header.ddspf.flgas & 0x40) {
    if (header.ddspf.redBitMask == 0xff) {
      iformat = header.ddspf.alphaBitMask ? GL_RGBA8 : GL_RGB8;
      format = header.ddspf.alphaBitMask ? GL_RGBA : GL_RGB;
    } else if (header.ddspf.blueBitMask == 0xff) {
      iformat = header.ddspf.alphaBitMask ? GL_RGBA8 : GL_RGB8;
      format = header.ddspf.alphaBitMask ? GL_BGRA : GL_BGR;
    } else {
      std::cerr << "WARNING: " << filename << "は未対応のDDSファイルです.\n";
      return 0;
    }
  } else {
    std::cerr << "WARNING: " << filename << "は未対応のDDSファイルです.\n";
    return 0;
  }

  const bool isCubemap = header.caps[1] & 0x200;
  const GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
  const int faceCount = isCubemap ? 6 : 1;

  GLuint texId;
  glGenTextures(1, &texId);
  glBindTexture(isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texId);

  std::vector<uint8_t> buf;
  buf.resize(fileSize - (128 + imageOffset));
  if (imageOffset) {
    ifs.seekg(imageOffset, std::ios_base::cur);
  }
  ifs.read(buf.data(), buf.size());
  const uint8_t* data = buf.data();
  for (int faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
    GLsizei curWidth = header.width;
    GLsizei curHeight = header.height;
    for (int mipLevel = 0; mipLevel < static_cast<int>(header.mipMapCount); ++mipLevel) {
      uint32_t imageSizeWithPadding;
      if (isCompressed) {
        imageSizeWithPadding = ((curWidth + 3) / 4) * ((curHeight + 3) / 4) * blockSize;
      } else {
        imageSizeWithPadding = curWidth * curHeight * 4;
      }
      if (isCompressed) {
        glCompressedTexImage2D(target + faceIndex, mipLevel, iformat, curWidth, curHeight, 0, imageSizeWithPadding, data);
      } else {
        glTexImage2D(target + faceIndex, mipLevel, iformat, curWidth, curHeight, 0, format, GL_UNSIGNED_BYTE, data);
      }
      const GLenum result = glGetError();
      switch (result) {
      case GL_NO_ERROR:
        break;
      case GL_INVALID_OPERATION:
        std::cerr << "WARNING: " << filename << "の読み込みに失敗.\n";
        break;
      default:
        std::cerr << "WARNING: " << filename << "の読み込みに失敗(" << std::hex << result << ").\n";
        break;
      }
      curWidth = std::max(1, curWidth / 2);
      curHeight = std::max(1, curHeight / 2);
      data += imageSizeWithPadding;
    }
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, header.mipMapCount - 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, header.mipMapCount <= 1 ? GL_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);
  return texId;
}

/**
* BMPファイルから2Dテクスチャを作成する.
*
* @param filename ファイル名.
*
* @return 作成に成功した場合はテクスチャポインタを返す.
*         失敗した場合はnullptr返す.
*/
bool LoadBMP(const char* filename, std::basic_ifstream<uint8_t>& ifs, size_t fileSize, ImageData& imageData)
{
  const size_t bmpFileHeaderSize = 14; // ビットマップファイルヘッダのバイト数
  const size_t windowsV1HeaderSize = 40; // ビットマップ情報ヘッダのバイト数.
  if (fileSize < bmpFileHeaderSize + windowsV1HeaderSize) {
    return false; // BMPファイルではない.
  }
  uint8_t bmpHeader[bmpFileHeaderSize + windowsV1HeaderSize];
  ifs.read(bmpHeader, sizeof(bmpHeader));

  if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
    return false; // BMPファイルではない.
  }
  const size_t offsetBytes = Get(bmpHeader, 10, 4);
  const uint32_t infoSize = Get(bmpHeader, 14, 4);
  const uint32_t width = Get(bmpHeader, 18, 4);
  const uint32_t height = Get(bmpHeader, 22, 4);
  const uint32_t bitCount = Get(bmpHeader, 28, 2);
  const uint32_t compression = Get(bmpHeader, 30, 4);
  const size_t pixelBytes = bitCount / 8;
  if (infoSize != windowsV1HeaderSize || compression) {
    std::cerr << "WARNING: " << filename << "は未対応のBMPファイルです.\n";
    return false; // 未対応のBMPファイル.
  }

  const size_t imageSize = width * height * pixelBytes;
  imageData.data.resize(imageSize);
  ifs.read(imageData.data.data(), imageSize);

  GLenum type = GL_UNSIGNED_BYTE;
  GLenum format = GL_BGR;
  if (bitCount == 8) {
    format = GL_RED;
  } else if (bitCount == 16) {
    type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
  }
  imageData.width = width;
  imageData.height = height;
  imageData.format = format;
  imageData.type = type;
  return true;
}

/**
* TGAファイルから2Dテクスチャを作成する.
*
* @param filename ファイル名.
*
* @return 作成に成功した場合はテクスチャポインタを返す.
*         失敗した場合はnullptr返す.
*/
bool LoadTGA(const char* path, std::basic_ifstream<uint8_t>& ifs, size_t fileSize, ImageData& imageData)
{
  // TGAヘッダを読み込む.
  uint8_t tgaHeader[18];
  ifs.read(tgaHeader, 18);

  // イメージIDを飛ばす.
  ifs.ignore(tgaHeader[0]);

  // カラーマップを飛ばす.
  if (tgaHeader[1]) {
    const int colorMapLength = tgaHeader[5] | (tgaHeader[6] << 8);
    const int colorMapEntrySize = tgaHeader[7];
    const int colorMapSize = colorMapLength * colorMapEntrySize / 8;
    ifs.ignore(colorMapSize);
  }

  // 画像データを読み込む.
  const int width = tgaHeader[12] | (tgaHeader[13] << 8);
  const int height = tgaHeader[14] | (tgaHeader[15] << 8);
  if (width <= 0 || height <= 0) {
    return false; // 未対応のTGAファイル、またはTGAふぁいるではない.
  }
  const int pixelDepth = tgaHeader[16];
  if (pixelDepth != 8 && pixelDepth != 16 && pixelDepth != 24 && pixelDepth != 32) {
    return false; // 未対応のTGAファイル、またはTGAふぁいるではない.
  }
  const int imageSize = width * height * pixelDepth / 8;
  if (fileSize < imageSize + sizeof(tgaHeader)) {
    return false; // 未対応のTGAファイル、またはTGAふぁいるではない.
  }
  imageData.data.resize(imageSize);
  ifs.read(imageData.data.data(), imageSize);

  // 画像データが「上から下」で格納されている場合、上下を入れ替える.
  if (tgaHeader[17] & 0x20) {
    std::cout << "反転中…";
    const int lineSize = width * pixelDepth / 8;
    std::vector<uint8_t> tmp(imageSize);
    std::vector<uint8_t>::iterator source = imageData.data.begin();
    std::vector<uint8_t>::iterator destination = tmp.end();
    for (int i = 0; i < height; ++i) {
      destination -= lineSize;
      std::copy(source, source + lineSize, destination);
      source += lineSize;
    }
    imageData.data.swap(tmp);
  }

  // 読み込んだ画像データからテクスチャを作成する.
  GLenum type = GL_UNSIGNED_BYTE;
  GLenum format = GL_BGRA;
  if (tgaHeader[2] == 3) {
    format = GL_RED;
  }
  if (tgaHeader[16] == 24) {
    format = GL_BGR;
  } else if (tgaHeader[16] == 16) {
    type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
  }
  imageData.width = width;
  imageData.height = height;
  imageData.type = type;
  imageData.format = format;
  return true;
}

/**
* ファイルから画像データを読み込む.
*
* @param path      画像として読み込むファイルのパス.
* @param imageData 画像データを格納する構造体.
*
* @retval true  読み込み成功.
* @retval false 読み込み失敗.
*/
bool LoadImage2D(const char* path, ImageData& imageData)
{
  std::basic_ifstream<uint8_t> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs) {
    std::cerr << "WARNING: " << path << "を開けません.\n";
    return 0;
  }
  std::vector<uint8_t> tmp(1024 * 1024);
  ifs.rdbuf()->pubsetbuf(tmp.data(), tmp.size());

  ifs.seekg(0, std::ios_base::end);
  const size_t size = static_cast<size_t>(ifs.tellg());

  std::cout << "INFO: " << path << "を読み込み中…";

  ifs.seekg(0, std::ios_base::beg);
  if (LoadBMP(path, ifs, size, imageData)) {
    std::cout << "完了\n";
    return true;
  }
  ifs.seekg(0, std::ios_base::beg);
  if (LoadTGA(path, ifs, size, imageData)) {
    std::cout << "完了\n";
    return true;
  }
  std::cout << "失敗\n";
  return false;
}

/**
* ファイルから2Dテクスチャを読み込む.
*
* @param path 2Dテクスチャとして読み込むファイルのパス.
*
* @retval 0以外  作成したテクスチャ・オブジェクトのID.
* @retval 0      テクスチャの作成に失敗.
*/
GLuint LoadImage2D(const char* path)
{
  std::basic_ifstream<uint8_t> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs) {
    std::cerr << "WARNING: " << path << "を開けません.\n";
    return 0;
  }
  std::vector<uint8_t> tmp(1024 * 1024);
  ifs.rdbuf()->pubsetbuf(tmp.data(), tmp.size());

  ifs.seekg(0, std::ios_base::end);
  const size_t size = static_cast<size_t>(ifs.tellg());

  std::cout << "INFO: " << path << "を読み込み中…";

  ImageData imageData;
  ifs.seekg(0, std::ios_base::beg);
  if (LoadBMP(path, ifs, size, imageData)) {
    std::cout << "完了\n";
    return CreateImage2D(imageData.width, imageData.height, imageData.data.data(), imageData.format, imageData.type);
  }
  ifs.seekg(0, std::ios_base::beg);
  if (LoadTGA(path, ifs, size, imageData)) {
    std::cout << "完了\n";
    return CreateImage2D(imageData.width, imageData.height, imageData.data.data(), imageData.format, imageData.type);
  }
  ifs.seekg(0, std::ios_base::beg);
  const GLuint texId = LoadDDS(path, ifs, size);
  if (texId) {
    std::cout << "完了\n";
    return texId;
  }
  std::cout << "失敗\n";
  return 0;
}

/**
* 2Dテクスチャを作成する.
*
* @param width   テクスチャの幅(ピクセル数).
* @param height  テクスチャの高さ(ピクセル数).
* @param data    テクスチャデータへのポインタ.
* @param format  転送元画像のデータ形式.
* @param type    転送元画像のデータ格納形式.
*
* @retval 0以外  作成したテクスチャ・オブジェクトのID.
* @retval 0      テクスチャの作成に失敗.
*/
GLuint CreateImage2D(GLsizei width, GLsizei height, const GLvoid* data, GLenum format, GLenum type)
{
  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLenum internalFormat = GL_RGBA8;
  if (format == GL_BGR) {
    internalFormat = GL_RGB8;
  } else if (format == GL_RED) {
    internalFormat = GL_R8;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  const GLenum result = glGetError();
  if (result != GL_NO_ERROR) {
    std::cerr << "ERROR: テクスチャの作成に失敗(0x" << std::hex << result << ").";
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &id);
    return 0;
  }

  // テクスチャのパラメータを設定する.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (format == GL_RED) {
    const GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  return id;
}

/**
* コンストラクタ.
*
* @param path テクスチャファイル名.
*/
Image2D::Image2D(const char* path)
{
  id = LoadImage2D(path);
  if (id) {
    const GLenum target = Target();
    glBindTexture(target, id);
    glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &height);
    glBindTexture(target, 0);
  }
}

/**
* デストラクタ.
*/
Image2D::~Image2D()
{
  glDeleteTextures(1, &id);
}

/**
* テクスチャ・オブジェクトが設定されているか調べる.
*
* @retval true  設定されている.
* @retval false 設定されていない.
*/
bool Image2D::IsNull() const
{
  return id;
}

/**
* テクスチャに対応するバインド対象を取得する.
*
* @return テクスチャに対応するバインド対象.
*         現在はGL_TEXTURE_2DとGL_TEXTURE_CUBE_MAPのいずれか.
*/
GLenum Image2D::Target() const
{
  return isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
}

/**
* テクスチャをバインドする.
*/
void Image2D::Bind(int no) const
{
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(Target(), Id());
}

/**
* テクスチャのバインドを解除する.
*/
void Image2D::Unbind(int no) const
{
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(Target(), 0);
}

/**
* 2Dテクスチャを作成する.
*
* @param path テクスチャファイル名.
*/
Image2DPtr Image2D::Create(const char* path)
{
  struct Impl : Image2D {
    Impl(const char* path) : Image2D(path) {}
  };
  return std::make_shared<Impl>(path);
}

} // namespace Texture