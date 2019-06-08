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

/// �e�N�X�`���֘A�̊֐���N���X���i�[���閼�O���.
namespace Texture {
/**
* FOURCC���쐬����.
*/
#define MAKE_FOURCC(a, b, c, d) static_cast<uint32_t>(a + (b << 8) + (c << 16) + (d << 24))

/**
* �o�C�g�񂩂琔�l�𕜌�����.
*
* @param p      �o�C�g��ւ̃|�C���^.
* @param offset ���l�̃I�t�Z�b�g.
* @param size   ���l�̃o�C�g��(1�`4).
*
* @return �����������l.
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
* DDS�摜���.
*/
struct DDSPixelFormat
{
  uint32_t size; ///< ���̍\���̂̃o�C�g��(32).
  uint32_t flgas; ///< �摜�Ɋ܂܂��f�[�^�̎�ނ������t���O.
  uint32_t fourCC; ///< �摜�t�H�[�}�b�g������FOURCC.
  uint32_t rgbBitCount; ///< 1�s�N�Z���̃r�b�g��.
  uint32_t redBitMask; ///< �ԗv�f���g�������������r�b�g.
  uint32_t greenBitMask; ///< �Ηv�f���g�������������r�b�g.
  uint32_t blueBitMask; ///< �v�f���g�������������r�b�g.
  uint32_t alphaBitMask; ///< �����v�f���g�������������r�b�g.
};

/**
* �o�b�t�@����DDS�摜����ǂݏo��.
*
* @param buf �ǂݏo�����o�b�t�@.
*
* @return �ǂݏo����DDS�摜���.
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
* DDS�t�@�C���w�b�_.
*/
struct DDSHeader
{
  uint32_t size;  ///< ���̍\���̂̃o�C�g��(124).
  uint32_t flags; ///< �ǂ̃p�����[�^���L�����������t���O.
  uint32_t height; ///< �摜�̍���(�s�N�Z����).
  uint32_t width; ///< �摜�̕�(�s�N�Z����).
  uint32_t pitchOrLinearSize; ///< ���̃o�C�g���܂��͉摜1���̃o�C�g��.
  uint32_t depth; ///< �摜�̉��s��(����)(3�����e�N�X�`�����Ŏg�p).
  uint32_t mipMapCount; ///< �܂܂�Ă���~�b�v�}�b�v���x����.
  uint32_t reserved1[11]; ///< (�����̂��߂ɗ\�񂳂�Ă���).
  DDSPixelFormat ddspf; ///< DDS�摜���.
  uint32_t caps[4]; ///< �܂܂�Ă���摜�̎��.
  uint32_t reserved2; ///< (�����̂��߂ɗ\�񂳂�Ă���).
};

/**
* �o�b�t�@����DDS�t�@�C���w�b�_��ǂݏo��.
*
* @param buf �ǂݏo�����o�b�t�@.
*
* @return �ǂݏo����DDS�t�@�C���w�b�_.
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
* DDS�t�@�C���w�b�_(DirectX10�g��).
*/
struct DDSHeaderDX10
{
  uint32_t dxgiFormat; ///< �摜�̎��(DX10�ȍ~�ɒǉ����ꂽ���).
  uint32_t resourceDimension; ///< ������(1D or 2D or 3D).
  uint32_t miscFlag; ///< �摜���z�肷��g�����������t���O.
  uint32_t arraySize; ///< �i�[����Ă���̂��e�N�X�`���z��̏ꍇ�A���̔z��T�C�Y.
  uint32_t reserved; ///< (�����̂��߂ɗ\�񂳂�Ă���).
};

/**
* �o�b�t�@����DDS�t�@�C���w�b�_(DirectX10�g��)��ǂݏo��.
*
* @param buf �ǂݏo�����o�b�t�@.
*
* @return �ǂݏo����DDS�t�@�C���w�b�_(DirectX10�g��).
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
* DDS�t�@�C������e�N�X�`�����쐬����.
*
* @param filename DDS�t�@�C����.
* @param st       DDS�t�@�C���X�e�[�^�X.
* @param buf      �t�@�C����ǂݍ��񂾃o�b�t�@.
* @param header   DDS�w�b�_�i�[��ւ̃|�C���^.
*
* @retval 0�ȊO �쐬�����e�N�X�`��ID.
* @retval 0     �쐬���s.
*/
GLuint LoadDDS(const char* filename, std::basic_ifstream<uint8_t>& ifs, size_t fileSize)
{
  if (fileSize < 128) {
    std::cerr << "WARNING: " << filename << "��DDS�t�@�C���ł͂���܂���.\n";
    return 0;
  }
  uint8_t ddsHeader[128];
  ifs.read(ddsHeader, sizeof(ddsHeader));
  const DDSHeader header = ReadDDSHeader(ddsHeader + 4);
  if (header.size != 124) {
    std::cerr << "WARNING: " << filename << "��DDS�t�@�C���ł͂���܂���.\n";
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
        std::cerr << "WARNING: " << filename << "�͖��Ή���DDS�t�@�C���ł�.\n";
        return 0;
      }
      imageOffset = 20; // DX10�w�b�_�̂Ԃ�����Z.
      break;
    }
    default:
      std::cerr << "WARNING: " << filename << "�͖��Ή���DDS�t�@�C���ł�.\n";
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
      std::cerr << "WARNING: " << filename << "�͖��Ή���DDS�t�@�C���ł�.\n";
      return 0;
    }
  } else {
    std::cerr << "WARNING: " << filename << "�͖��Ή���DDS�t�@�C���ł�.\n";
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
        std::cerr << "WARNING: " << filename << "�̓ǂݍ��݂Ɏ��s.\n";
        break;
      default:
        std::cerr << "WARNING: " << filename << "�̓ǂݍ��݂Ɏ��s(" << std::hex << result << ").\n";
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
* BMP�t�@�C������2D�e�N�X�`�����쐬����.
*
* @param filename �t�@�C����.
*
* @return �쐬�ɐ��������ꍇ�̓e�N�X�`���|�C���^��Ԃ�.
*         ���s�����ꍇ��nullptr�Ԃ�.
*/
bool LoadBMP(const char* filename, std::basic_ifstream<uint8_t>& ifs, size_t fileSize, ImageData& imageData)
{
  const size_t bmpFileHeaderSize = 14; // �r�b�g�}�b�v�t�@�C���w�b�_�̃o�C�g��
  const size_t windowsV1HeaderSize = 40; // �r�b�g�}�b�v���w�b�_�̃o�C�g��.
  if (fileSize < bmpFileHeaderSize + windowsV1HeaderSize) {
    return false; // BMP�t�@�C���ł͂Ȃ�.
  }
  uint8_t bmpHeader[bmpFileHeaderSize + windowsV1HeaderSize];
  ifs.read(bmpHeader, sizeof(bmpHeader));

  if (bmpHeader[0] != 'B' || bmpHeader[1] != 'M') {
    return false; // BMP�t�@�C���ł͂Ȃ�.
  }
  const size_t offsetBytes = Get(bmpHeader, 10, 4);
  const uint32_t infoSize = Get(bmpHeader, 14, 4);
  const uint32_t width = Get(bmpHeader, 18, 4);
  const uint32_t height = Get(bmpHeader, 22, 4);
  const uint32_t bitCount = Get(bmpHeader, 28, 2);
  const uint32_t compression = Get(bmpHeader, 30, 4);
  const size_t pixelBytes = bitCount / 8;
  if (infoSize != windowsV1HeaderSize || compression) {
    std::cerr << "WARNING: " << filename << "�͖��Ή���BMP�t�@�C���ł�.\n";
    return false; // ���Ή���BMP�t�@�C��.
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
* TGA�t�@�C������2D�e�N�X�`�����쐬����.
*
* @param filename �t�@�C����.
*
* @return �쐬�ɐ��������ꍇ�̓e�N�X�`���|�C���^��Ԃ�.
*         ���s�����ꍇ��nullptr�Ԃ�.
*/
bool LoadTGA(const char* path, std::basic_ifstream<uint8_t>& ifs, size_t fileSize, ImageData& imageData)
{
  // TGA�w�b�_��ǂݍ���.
  uint8_t tgaHeader[18];
  ifs.read(tgaHeader, 18);

  // �C���[�WID���΂�.
  ifs.ignore(tgaHeader[0]);

  // �J���[�}�b�v���΂�.
  if (tgaHeader[1]) {
    const int colorMapLength = tgaHeader[5] | (tgaHeader[6] << 8);
    const int colorMapEntrySize = tgaHeader[7];
    const int colorMapSize = colorMapLength * colorMapEntrySize / 8;
    ifs.ignore(colorMapSize);
  }

  // �摜�f�[�^��ǂݍ���.
  const int width = tgaHeader[12] | (tgaHeader[13] << 8);
  const int height = tgaHeader[14] | (tgaHeader[15] << 8);
  if (width <= 0 || height <= 0) {
    return false; // ���Ή���TGA�t�@�C���A�܂���TGA�ӂ�����ł͂Ȃ�.
  }
  const int pixelDepth = tgaHeader[16];
  if (pixelDepth != 8 && pixelDepth != 16 && pixelDepth != 24 && pixelDepth != 32) {
    return false; // ���Ή���TGA�t�@�C���A�܂���TGA�ӂ�����ł͂Ȃ�.
  }
  const int imageSize = width * height * pixelDepth / 8;
  if (fileSize < imageSize + sizeof(tgaHeader)) {
    return false; // ���Ή���TGA�t�@�C���A�܂���TGA�ӂ�����ł͂Ȃ�.
  }
  imageData.data.resize(imageSize);
  ifs.read(imageData.data.data(), imageSize);

  // �摜�f�[�^���u�ォ�牺�v�Ŋi�[����Ă���ꍇ�A�㉺�����ւ���.
  if (tgaHeader[17] & 0x20) {
    std::cout << "���]���c";
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

  // �ǂݍ��񂾉摜�f�[�^����e�N�X�`�����쐬����.
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
* �t�@�C������摜�f�[�^��ǂݍ���.
*
* @param path      �摜�Ƃ��ēǂݍ��ރt�@�C���̃p�X.
* @param imageData �摜�f�[�^���i�[����\����.
*
* @retval true  �ǂݍ��ݐ���.
* @retval false �ǂݍ��ݎ��s.
*/
bool LoadImage2D(const char* path, ImageData& imageData)
{
  std::basic_ifstream<uint8_t> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs) {
    std::cerr << "WARNING: " << path << "���J���܂���.\n";
    return 0;
  }
  std::vector<uint8_t> tmp(1024 * 1024);
  ifs.rdbuf()->pubsetbuf(tmp.data(), tmp.size());

  ifs.seekg(0, std::ios_base::end);
  const size_t size = static_cast<size_t>(ifs.tellg());

  std::cout << "INFO: " << path << "��ǂݍ��ݒ��c";

  ifs.seekg(0, std::ios_base::beg);
  if (LoadBMP(path, ifs, size, imageData)) {
    std::cout << "����\n";
    return true;
  }
  ifs.seekg(0, std::ios_base::beg);
  if (LoadTGA(path, ifs, size, imageData)) {
    std::cout << "����\n";
    return true;
  }
  std::cout << "���s\n";
  return false;
}

/**
* �t�@�C������2D�e�N�X�`����ǂݍ���.
*
* @param path 2D�e�N�X�`���Ƃ��ēǂݍ��ރt�@�C���̃p�X.
*
* @retval 0�ȊO  �쐬�����e�N�X�`���E�I�u�W�F�N�g��ID.
* @retval 0      �e�N�X�`���̍쐬�Ɏ��s.
*/
GLuint LoadImage2D(const char* path)
{
  std::basic_ifstream<uint8_t> ifs;
  ifs.open(path, std::ios_base::binary);
  if (!ifs) {
    std::cerr << "WARNING: " << path << "���J���܂���.\n";
    return 0;
  }
  std::vector<uint8_t> tmp(1024 * 1024);
  ifs.rdbuf()->pubsetbuf(tmp.data(), tmp.size());

  ifs.seekg(0, std::ios_base::end);
  const size_t size = static_cast<size_t>(ifs.tellg());

  std::cout << "INFO: " << path << "��ǂݍ��ݒ��c";

  ImageData imageData;
  ifs.seekg(0, std::ios_base::beg);
  if (LoadBMP(path, ifs, size, imageData)) {
    std::cout << "����\n";
    return CreateImage2D(imageData.width, imageData.height, imageData.data.data(), imageData.format, imageData.type);
  }
  ifs.seekg(0, std::ios_base::beg);
  if (LoadTGA(path, ifs, size, imageData)) {
    std::cout << "����\n";
    return CreateImage2D(imageData.width, imageData.height, imageData.data.data(), imageData.format, imageData.type);
  }
  ifs.seekg(0, std::ios_base::beg);
  const GLuint texId = LoadDDS(path, ifs, size);
  if (texId) {
    std::cout << "����\n";
    return texId;
  }
  std::cout << "���s\n";
  return 0;
}

/**
* 2D�e�N�X�`�����쐬����.
*
* @param width   �e�N�X�`���̕�(�s�N�Z����).
* @param height  �e�N�X�`���̍���(�s�N�Z����).
* @param data    �e�N�X�`���f�[�^�ւ̃|�C���^.
* @param format  �]�����摜�̃f�[�^�`��.
* @param type    �]�����摜�̃f�[�^�i�[�`��.
*
* @retval 0�ȊO  �쐬�����e�N�X�`���E�I�u�W�F�N�g��ID.
* @retval 0      �e�N�X�`���̍쐬�Ɏ��s.
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
    std::cerr << "ERROR: �e�N�X�`���̍쐬�Ɏ��s(0x" << std::hex << result << ").";
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &id);
    return 0;
  }

  // �e�N�X�`���̃p�����[�^��ݒ肷��.
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
* �R���X�g���N�^.
*
* @param path �e�N�X�`���t�@�C����.
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
* �f�X�g���N�^.
*/
Image2D::~Image2D()
{
  glDeleteTextures(1, &id);
}

/**
* �e�N�X�`���E�I�u�W�F�N�g���ݒ肳��Ă��邩���ׂ�.
*
* @retval true  �ݒ肳��Ă���.
* @retval false �ݒ肳��Ă��Ȃ�.
*/
bool Image2D::IsNull() const
{
  return id;
}

/**
* �e�N�X�`���ɑΉ�����o�C���h�Ώۂ��擾����.
*
* @return �e�N�X�`���ɑΉ�����o�C���h�Ώ�.
*         ���݂�GL_TEXTURE_2D��GL_TEXTURE_CUBE_MAP�̂����ꂩ.
*/
GLenum Image2D::Target() const
{
  return isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
}

/**
* �e�N�X�`�����o�C���h����.
*/
void Image2D::Bind(int no) const
{
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(Target(), Id());
}

/**
* �e�N�X�`���̃o�C���h����������.
*/
void Image2D::Unbind(int no) const
{
  glActiveTexture(GL_TEXTURE0 + no);
  glBindTexture(Target(), 0);
}

/**
* 2D�e�N�X�`�����쐬����.
*
* @param path �e�N�X�`���t�@�C����.
*/
Image2DPtr Image2D::Create(const char* path)
{
  struct Impl : Image2D {
    Impl(const char* path) : Image2D(path) {}
  };
  return std::make_shared<Impl>(path);
}

} // namespace Texture