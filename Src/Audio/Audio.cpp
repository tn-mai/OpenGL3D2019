/**
* @file Audio.cpp
*/
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include "Audio.h"
#include <xaudio2.h>
#include <vector>
#include <list>
#include <stdint.h>
#include <wrl/client.h>
#include <algorithm>
#include <wincodec.h>
#include <iostream>

#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "xaudio2.lib")

using Microsoft::WRL::ComPtr;

/**
* �����֘A�̃R�[�h���i�[���閼�O���.
*/
namespace Audio {

class EngineImpl;
using EngineImplPtr = std::shared_ptr<EngineImpl>;

struct MediaFoundationInitialize {
  MediaFoundationInitialize() {
    success = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
  }
  ~MediaFoundationInitialize() {
    if (success) {
      MFShutdown();
    }
  }
  bool success;
};

const uint32_t FOURCC_RIFF_TAG = MAKEFOURCC('R', 'I', 'F', 'F');
const uint32_t FOURCC_FORMAT_TAG = MAKEFOURCC('f', 'm', 't', ' ');
const uint32_t FOURCC_DATA_TAG = MAKEFOURCC('d', 'a', 't', 'a');
const uint32_t FOURCC_WAVE_FILE_TAG = MAKEFOURCC('W', 'A', 'V', 'E');
const uint32_t FOURCC_XWMA_FILE_TAG = MAKEFOURCC('X', 'W', 'M', 'A');
const uint32_t FOURCC_XWMA_DPDS = MAKEFOURCC('d', 'p', 'd', 's');

struct RIFFChunk
{
  uint32_t tag;
  uint32_t size;
};

/**
* WAV�f�[�^.
*/
struct WF
{
  union U {
    WAVEFORMATEXTENSIBLE ext;
    struct ADPCMWAVEFORMAT {
      WAVEFORMATEX    wfx;
      WORD            wSamplesPerBlock;
      WORD            wNumCoef;
      ADPCMCOEFSET    coef[7];
    } adpcm;
  } u;
  size_t dataOffset;
  size_t dataSize;
  size_t seekOffset;
  size_t seekSize;
};

typedef std::vector<uint8_t> BufferType;

struct ScopedHandle
{
  ScopedHandle(HANDLE h) : handle(h == INVALID_HANDLE_VALUE ? 0 : h) {}
  ~ScopedHandle() { if (handle) { CloseHandle(handle); } }
  operator HANDLE() { return handle; }
  HANDLE handle;
};

bool Read(HANDLE hFile, void* buf, DWORD size)
{
  DWORD readSize;
  if (!ReadFile(hFile, buf, size, &readSize, nullptr) || readSize != size) {
    return false;
  }
  return true;
}

uint32_t GetWaveFormatTag(const WAVEFORMATEXTENSIBLE& wf)
{
  if (wf.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
    return wf.Format.wFormatTag;
  }
  return wf.SubFormat.Data1;
}

// �t�H�[�}�b�g�����擾
bool LoadWaveFile(HANDLE hFile, WF& wf, std::vector<UINT32>& seekTable, std::vector<uint8_t>* source)
{
  RIFFChunk riffChunk;
  if (!Read(hFile, &riffChunk, sizeof(riffChunk))) {
    return false;
  }
  if (riffChunk.tag != FOURCC_RIFF_TAG) {
    return false;
  }

  uint32_t fourcc;
  if (!Read(hFile, &fourcc, sizeof(fourcc))) {
    return false;
  }
  if (fourcc != FOURCC_WAVE_FILE_TAG && fourcc != FOURCC_XWMA_FILE_TAG) {
    return false;
  }

  bool hasWaveFormat = false;
  bool hasData = false;
  bool hasDpds = false;
  size_t offset = 12;
  do {
    if (SetFilePointer(hFile, offset, nullptr, FILE_BEGIN) != offset) {
      return false;
    }

    RIFFChunk chunk;
    if (!Read(hFile, &chunk, sizeof(chunk))) {
      break;
    }

    if (chunk.tag == FOURCC_FORMAT_TAG) {
      if (!Read(hFile, &wf.u, std::min(chunk.size, sizeof(WF::U)))) {
        break;
      }
      switch (GetWaveFormatTag(wf.u.ext)) {
      case WAVE_FORMAT_PCM:
        wf.u.ext.Format.cbSize = 0;
        /* FALLTHROUGH */
      case WAVE_FORMAT_IEEE_FLOAT:
      case WAVE_FORMAT_ADPCM:
        wf.seekSize = 0;
        wf.seekOffset = 0;
        hasDpds = true;
        break;
      case WAVE_FORMAT_WMAUDIO2:
      case WAVE_FORMAT_WMAUDIO3:
        break;
      default:
        // ���̃R�[�h�ŃT�|�[�g���Ȃ��t�H�[�}�b�g.
        return false;
      }
      hasWaveFormat = true;
    }
    else if (chunk.tag == FOURCC_DATA_TAG) {
      wf.dataOffset = offset + sizeof(RIFFChunk);
      wf.dataSize = chunk.size;
      hasData = true;
    }
    else if (chunk.tag == FOURCC_XWMA_DPDS) {
      wf.seekOffset = offset + sizeof(RIFFChunk);
      wf.seekSize = chunk.size / 4;
      hasDpds = true;
    }
    offset += chunk.size + sizeof(RIFFChunk);
  } while (!hasWaveFormat || !hasData || !hasDpds);
  if (!(hasWaveFormat && hasData && hasDpds)) {
    return false;
  }

  if (wf.seekSize) {
    seekTable.resize(wf.seekSize);
    SetFilePointer(hFile, wf.seekOffset, nullptr, FILE_BEGIN);
    if (!Read(hFile, seekTable.data(), wf.seekSize * 4)) {
      return false;
    }
  }
  if (source) {
    source->resize(wf.dataSize);
    SetFilePointer(hFile, wf.dataOffset, nullptr, FILE_BEGIN);
    if (!Read(hFile, source->data(), wf.dataSize)) {
      return false;
    }
  }
  return true;
}

/**
* Sound�̋����.
*
* �I�[�f�B�I�t�@�C���̃p�X����������������A�Đ��ł��Ȃ��t�H�[�}�b�g�������肵��
* �ꍇ�ɁA���̃N���X�̃I�u�W�F�N�g���Ԃ����.
*/
class NullSoundImpl : public Sound
{
public:
  virtual ~NullSoundImpl() = default;
  virtual bool Play(int) override { return false; }
  virtual bool Pause() override { return false; }
  virtual bool Seek() override { return false; }
  virtual bool Stop() override { return false; }
  virtual float SetVolume(float) override { return 0; }
  virtual float SetPitch(float) override { return 0; }
  virtual int GetState() const override { return 0; }
  virtual float GetVolume() const override { return 0; }
  virtual float GetPitch() const override { return 0; }
  virtual bool IsNull() const override { return true; }
};

/**
* Sound�̎���.
*
* �X�g���[�~���O���s��Ȃ��I�[�f�B�I�N���X.
* SE�ɓK���Ă���.
* �Ή��`��: WAV, XWM.
*/
class SoundImpl : public Sound
{
public:
  SoundImpl() : state(State_Create), sourceVoice(nullptr) {
  }

  virtual ~SoundImpl() override {
    if (sourceVoice) {
      sourceVoice->DestroyVoice();
    }
  }

  virtual bool Play(int flags) override {
    if (!sourceVoice) {
      return false;
    }
    if (!(state & State_Pausing)) {
      Stop();
      XAUDIO2_BUFFER buffer = {};
      buffer.Flags = XAUDIO2_END_OF_STREAM;
      buffer.AudioBytes = source.size();
      buffer.pAudioData = source.data();
      buffer.LoopCount = flags & Flag_Loop ? XAUDIO2_LOOP_INFINITE : XAUDIO2_NO_LOOP_REGION;
      if (seekTable.empty()) {
        if (FAILED(sourceVoice->SubmitSourceBuffer(&buffer))) {
          return false;
        }
      } else {
        const XAUDIO2_BUFFER_WMA seekInfo = { seekTable.data(), seekTable.size() };
        if (FAILED(sourceVoice->SubmitSourceBuffer(&buffer, &seekInfo))) {
          return false;
        }
      }
    }
    state = State_Playing;
    return SUCCEEDED(sourceVoice->Start());
  }

  virtual bool Pause() override {
    if (sourceVoice && (state & State_Playing)) {
      state |= State_Pausing;
      return SUCCEEDED(sourceVoice->Stop());
    }
    return false;
  }

  virtual bool Seek() override {
    return sourceVoice;
  }

  virtual bool Stop() override {
    if (sourceVoice && (state & State_Playing)) {
      if (!(state & State_Pausing) && FAILED(sourceVoice->Stop())) {
        return false;
      }
      state = State_Stopped;
      return SUCCEEDED(sourceVoice->FlushSourceBuffers());
    }
    return false;
  }

  virtual float SetVolume(float volume) override {
    if (sourceVoice) {
      sourceVoice->SetVolume(volume);
    }
    return volume;
  }

  virtual float SetPitch(float pitch) override {
    if (sourceVoice) {
      sourceVoice->SetFrequencyRatio(pitch);
    }
    return pitch;
  }

  virtual int GetState() const override {
    if (!sourceVoice) {
      return State_Failed;
    }
    XAUDIO2_VOICE_STATE s;
    sourceVoice->GetState(&s);
    return s.BuffersQueued ? state : (State_Stopped | State_Prepared);
  }

  virtual float GetVolume() const override {
    float volume = 0;
    if (sourceVoice) {
      sourceVoice->GetVolume(&volume);
    }
    return volume;
  }

  virtual float GetPitch() const override {
    float ratio = 0;
    if (sourceVoice) {
      sourceVoice->GetFrequencyRatio(&ratio);
    }
    return ratio;
  }

  virtual bool IsNull() const override { return false; }

  int state;
  EngineImplPtr engine;
  IXAudio2SourceVoice* sourceVoice;
  std::vector<uint8_t> source;
  std::vector<UINT32> seekTable;
};

/**
* Sound�̎���.
*
* �X�g���[�~���O���s���I�[�f�B�I�N���X.
* BGM�ɓK���Ă���.
* �Ή��`��: WAV, XWM.
*/
class StreamSoundImpl : public Sound
{
public:
  StreamSoundImpl() = delete;
  explicit StreamSoundImpl(HANDLE h) :
    sourceVoice(nullptr), handle(h), state(State_Create), loop(false), currentPos(0), curBuf(0) {
    buf.resize(MAX_BUFFER_COUNT);
  }

  virtual ~StreamSoundImpl() override {
    if (sourceVoice) {
      sourceVoice->DestroyVoice();
    }
  }
  virtual bool Play(int flags) override {
    if (!sourceVoice) {
      return false;
    }
    if (!(state & State_Pausing)) {
      Stop();
    }
    state = State_Playing;
    loop = flags & Flag_Loop;
    return SUCCEEDED(sourceVoice->Start());
  }
  virtual bool Pause() override {
    if (sourceVoice && (state & State_Playing) && !(state & State_Pausing)) {
      state |= State_Pausing;
      return SUCCEEDED(sourceVoice->Stop());
    }
    return false;
  }
  virtual bool Seek() override {
    return sourceVoice;
  }
  virtual bool Stop() override {
    if (sourceVoice && (state & State_Playing)) {
      if (!(state & State_Pausing) && FAILED(sourceVoice->Stop())) {
        return false;
      }
      state = State_Stopped;
            lastSeekValue = 0;
            currentPos = 0;
            curBuf = 0;
      return SUCCEEDED(sourceVoice->FlushSourceBuffers());
    }
    return false;
  }
  virtual float SetVolume(float volume) override {
    if (sourceVoice) {
      sourceVoice->SetVolume(volume);
    }
    return volume;
  }
  virtual float SetPitch(float pitch) override {
    if (sourceVoice) {
      sourceVoice->SetFrequencyRatio(pitch);
    }
    return pitch;
  }
  virtual int GetState() const override {
    if (!sourceVoice) {
      return State_Failed;
    }
    XAUDIO2_VOICE_STATE s;
    sourceVoice->GetState(&s);
    return s.BuffersQueued ? (state | State_Prepared) : State_Stopped;
  }

  virtual float GetVolume() const override {
    float volume = 0;
    if (sourceVoice) {
      sourceVoice->GetVolume(&volume);
    }
    return volume;
  }

  virtual float GetPitch() const override {
    float ratio = 0;
    if (sourceVoice) {
      sourceVoice->GetFrequencyRatio(&ratio);
    }
    return ratio;
  }

  bool Update() {
    if (!sourceVoice) {
      return false;
    }
    const DWORD cbValid = std::min(packedAlignedBufferSize, dataSize - currentPos);
    if (cbValid == 0) {
      return false;
    }
    XAUDIO2_VOICE_STATE state;
    sourceVoice->GetState(&state);
    if (state.BuffersQueued >= MAX_BUFFER_COUNT - 1) {
      return true;
    }

    SetFilePointer(handle, dataOffset + currentPos, nullptr, FILE_BEGIN);
    if (!Read(handle, buf[curBuf].data, cbValid)) {
      return false;
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = buf[curBuf].data;
    buffer.AudioBytes = cbValid;
    buffer.Flags = cbValid == packedAlignedBufferSize ? 0 : XAUDIO2_END_OF_STREAM;
    if (seekTable.empty()) {
      sourceVoice->SubmitSourceBuffer(&buffer, nullptr);
    } else {
      XAUDIO2_BUFFER_WMA bufWma = {};
      bufWma.PacketCount = cbValid / packetSize;
      if (bufWma.PacketCount >= 0x100) {
        std::cerr << "WARNING: PacketCount���o�b�t�@�T�C�Y���z����." << std::endl;
      }
      bufWma.pDecodedPacketCumulativeBytes = buf[curBuf].seekTable;
      const UINT32* pSeekTable = seekTable.data() + (currentPos / packetSize);
      for (UINT32 i = 0; i < bufWma.PacketCount; ++i) {
        buf[curBuf].seekTable[i] = pSeekTable[i] - lastSeekValue;
      }
      lastSeekValue = bufWma.pDecodedPacketCumulativeBytes[bufWma.PacketCount - 1];
      sourceVoice->SubmitSourceBuffer(&buffer, &bufWma);
    }
    currentPos += buffer.AudioBytes;
    curBuf = (curBuf + 1) % MAX_BUFFER_COUNT;
    if (loop && currentPos >= dataSize) {
      currentPos = 0;
    }
    return true;
  }

  virtual bool IsNull() const override { return false; }

public:
  EngineImplPtr engine;
  IXAudio2SourceVoice* sourceVoice;
  std::vector<UINT32> seekTable;
  ScopedHandle handle;
  size_t dataSize = 0;
  size_t dataOffset = 0;
  size_t packetSize = 0;
  size_t packedAlignedBufferSize = 0;

  static const size_t BUFFER_SIZE = 0x10000;
  static const int MAX_BUFFER_COUNT = 3;
  struct Sample {
    uint8_t data[BUFFER_SIZE];
    UINT32 seekTable[0x100];
    DWORD length;
  };
  UINT32 lastSeekValue = 0;

  int state = 0;
  bool loop = false;
  std::vector<Sample> buf;
  size_t curBuf;
  size_t currentPos;
};

/**
* Media Foundation�𗘗p�����X�g���[���T�E���h�̎���.
*
* �X�g���[�~���O���s���I�[�f�B�I�N���X.
* BGM�ɓK���Ă���.
* �Ή��`��: WAV, MP3, AAC, WMA.
*/
class MFStreamSoundImpl : public Sound
{
public:
  MFStreamSoundImpl() = default;
  bool Init(ComPtr<IXAudio2> xaudio, IMFAttributes* attributes, const wchar_t* filename) {
    buf.resize(MAX_BUFFER_COUNT);
    curBuf = 0;
    // open media file.
    if (FAILED(MFCreateSourceReaderFromURL(filename, attributes, sourceReader.GetAddressOf()))) {
      return false;
    }
    ComPtr<IMFMediaType> nativeMediaType;
    if (FAILED(sourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nativeMediaType.GetAddressOf()))) {
      return false;
    }
    GUID majorType{};
    if (FAILED(nativeMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))) {
      return false;
    }
    if (majorType != MFMediaType_Audio) {
      return false;
    }
    GUID subType{};
    if (FAILED(nativeMediaType->GetGUID(MF_MT_SUBTYPE, &subType))) {
      return false;
    }
    if (subType == MFAudioFormat_Float || subType == MFAudioFormat_PCM) {
      // uncompressed format.
    } else {
      // compressed format.
      ComPtr<IMFMediaType> partialMediaType;
      if (FAILED(MFCreateMediaType(partialMediaType.GetAddressOf()))) {
        return false;
      }
      if (FAILED(partialMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))) {
        return false;
      }
      if (FAILED(partialMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM))) {
        return false;
      }
      if (FAILED(sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, partialMediaType.Get()))) {
        return false;
      }
    }
    ComPtr<IMFMediaType> uncompressedMediaType;
    if (FAILED(sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, uncompressedMediaType.GetAddressOf()))) {
      return false;
    }
    WAVEFORMATEX* pWaveFormatEx;
    uint32_t waveFormatLength;
    if (FAILED(MFCreateWaveFormatExFromMFMediaType(uncompressedMediaType.Get(), &pWaveFormatEx, &waveFormatLength))) {
      return false;
    }

    if (FAILED(xaudio->CreateSourceVoice(&sourceVoice, pWaveFormatEx))) {
      return false;
    }
    CoTaskMemFree(pWaveFormatEx);
    return true;
  }

  virtual ~MFStreamSoundImpl() override {
    if (sourceVoice) {
      sourceVoice->DestroyVoice();
    }
  }

  virtual bool Play(int flags) override {
    if (!sourceVoice) {
      return false;
    }
    if (!(state & State_Pausing)) {
      Stop();
    }
    isEndOfStream = false;
    state = State_Playing;
    loop = flags & Flag_Loop;
    return SUCCEEDED(sourceVoice->Start());
  }

  virtual bool Pause() override {
    if (sourceVoice && (state & State_Playing) && !(state & State_Pausing)) {
      state |= State_Pausing;
      return SUCCEEDED(sourceVoice->Stop());
    }
    return false;
  }

  virtual bool Seek() override {
    return sourceVoice;
  }

  virtual bool Stop() override {
    if (sourceVoice && (state & State_Playing)) {
      if (!(state & State_Pausing)) {
        if (FAILED(sourceVoice->Stop())) {
          return false;
        }
      }
      state = State_Stopped;
      isEndOfStream = false;
      curBuf = 0;
      currentPos = 0;
      return SUCCEEDED(sourceVoice->FlushSourceBuffers());
    }
    return false;
  }

  virtual float SetVolume(float volume) override {
    if (sourceVoice) {
      sourceVoice->SetVolume(volume);
    }
    return volume;
  }

  virtual float SetPitch(float pitch) override {
    if (sourceVoice) {
      sourceVoice->SetFrequencyRatio(pitch);
    }
    return pitch;
  }

  virtual int GetState() const override {
    if (!sourceVoice) {
      return State_Failed;
    }
    XAUDIO2_VOICE_STATE s;
    sourceVoice->GetState(&s);
    return s.BuffersQueued ? (state | State_Prepared) : State_Stopped;
  }

  virtual float GetVolume() const override {
    float volume = 0;
    if (sourceVoice) {
      sourceVoice->GetVolume(&volume);
    }
    return volume;
  }

  virtual float GetPitch() const override {
    float ratio = 0;
    if (sourceVoice) {
      sourceVoice->GetFrequencyRatio(&ratio);
    }
    return ratio;
  }

  virtual bool IsNull() const override { return false; }

  enum Result {
    success,
    endOfStream,
    readError,
  };

  Result ReadFile() {
    ComPtr<IMFSample> sample;
    DWORD flags = 0;
    if (FAILED(sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, sample.GetAddressOf()))) {
      return readError;
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
      // reached to the end of stream.
      return endOfStream;
    }
    ComPtr<IMFMediaBuffer> buffer;
    if (FAILED(sample->ConvertToContiguousBuffer(buffer.GetAddressOf()))) {
      return readError;
    }
    BYTE* pAudioData = nullptr;
    if (FAILED(buffer->Lock(&pAudioData, nullptr, &buf[curBuf].length))) {
      return readError;
    }
    if (buf[curBuf].length > BUFFER_SIZE) {
      return readError;
    }
    std::copy(pAudioData, pAudioData + buf[curBuf].length, buf[curBuf].data);
    if (FAILED(buffer->Unlock())) {
      return readError;
    }
    return success;
  }

  bool Update() {
    if (isEndOfStream) {
      return false;
    }
    if (!sourceVoice) {
      return false;
    }

    XAUDIO2_VOICE_STATE voiceState;
    sourceVoice->GetState(&voiceState);
    if (voiceState.BuffersQueued >= MAX_BUFFER_COUNT - 1) {
      return true;
    }

    Result result = ReadFile();
    if (result == readError) {
      return false;
    }
    if (result == endOfStream) {
      if (loop) {
        PROPVARIANT value;
        value.vt = VT_I8;
        value.hVal.QuadPart = 0;
        sourceReader->SetCurrentPosition(GUID_NULL, value);
        result = ReadFile();
        if (result == readError) {
          return false;
        }
      } else {
        isEndOfStream = true;
        return true;
      }
    }

    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = buf[curBuf].data;
    buffer.AudioBytes = buf[curBuf].length;
    if (!isEndOfStream) {
      buffer.Flags = 0;
    } else {
      buffer.Flags = XAUDIO2_END_OF_STREAM;
    }
    sourceVoice->SubmitSourceBuffer(&buffer, nullptr);

    currentPos += buffer.AudioBytes;
    curBuf = (curBuf + 1) % MAX_BUFFER_COUNT;
    return true;
  }

  ComPtr<IMFSourceReader> sourceReader;
  EngineImplPtr engine;
  IXAudio2SourceVoice* sourceVoice = nullptr;

  static const size_t BUFFER_SIZE = 0x20000;
  static const int MAX_BUFFER_COUNT = 3;
  struct Sample {
    uint8_t data[BUFFER_SIZE];
    DWORD length;
  };

  int state = State_Create;
  bool loop = false;
  bool isEndOfStream = false;
  std::vector<Sample> buf;
  size_t curBuf = 0;
  size_t currentPos = 0;
};

/**
* Engine�̎���.
*/
class EngineImpl : public Engine, public std::enable_shared_from_this<EngineImpl>
{
public:
//  EngineImpl() : Engine(), xaudio(), masteringVoice(nullptr) {}
//  virtual ~EngineImpl() {}

  /**
  * �����Đ��G���W��������������.
  *
  * @retval true  ����������.
  * @retval false ���������s.
  */
  virtual bool Initialize() override {
    if (xaudio) {
      std::cerr << "WARNING: Audio::Engine�͏������ς݂ł�." << std::endl;
      return true;
    }
    ComPtr<IXAudio2> tmpAudio;
    UINT32 flags = 0;
#ifndef NDEBUG
    //    flags |= XAUDIO2_DEBUG_ENGINE;
#endif // NDEBUG
    HRESULT hr = XAudio2Create(&tmpAudio, flags);
    if (FAILED(hr)) {
      std::cerr << "ERROR: XAudio2�̍쐬�Ɏ��s." << std::endl;
      return false;
    }
    if (1) {
      XAUDIO2_DEBUG_CONFIGURATION debug = {};
      debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS | XAUDIO2_LOG_MEMORY;
      debug.BreakMask = XAUDIO2_LOG_ERRORS;
      debug.LogFunctionName = TRUE;
      tmpAudio->SetDebugConfiguration(&debug);
    }
    hr = tmpAudio->CreateMasteringVoice(&masteringVoice);
    if (FAILED(hr)) {
      std::cerr << "ERROR: XAudio2�̉��ʐݒ�Ɏ��s." << std::endl;
      return false;
    }

    // Setup for Media foundation.
    mf = std::make_unique<MediaFoundationInitialize>();
    if (FAILED(MFCreateAttributes(attributes.GetAddressOf(), 1))) {
      std::cerr << "ERROR: Media Foundation�̑����I�u�W�F�N�g�̍쐬�Ɏ��s." << std::endl;
      return false;
    }
    if (FAILED(attributes->SetUINT32(MF_LOW_LATENCY, true))) {
      std::cerr << "ERROR: Media Foundation�̃��C�e���V�̐ݒ�Ɏ��s." << std::endl;
      return false;
    }
    xaudio.Swap(std::move(tmpAudio));

    return true;
  }

  /**
  * �G���W����j������.
  */
  virtual void Finalize() override {
    streamSound.reset();
    soundList.clear();
    mfSoundList.clear();
  }

  /**
  * �G���W���̏�Ԃ��X�V����.
  *
  * ����I�ɌĂяo���K�v������.
  *
  * @retval true  �X�V����.
  * @retval false �X�V���s.
  *
  * @note ���݂͏��true��Ԃ�.
  */
  virtual bool Update() override {
    soundList.remove_if(
      [](const SoundList::value_type& p) { return (p.use_count() <= 1) && (p->GetState() & State_Stopped); }
    );

    for (auto&& e : mfSoundList) {
      e->Update();
    }
    mfSoundList.remove_if(
      [](const MFSoundList::value_type& p) { return (p.use_count() <= 1) && (p->GetState() & State_Stopped); }
    );

    if (streamSound) {
      streamSound->Update();
      if ((streamSound.use_count() <= 1) && (streamSound->GetState() & State_Stopped)) {
        streamSound.reset();
      }
    }
    return true;
  }

  /**
  * ��������������(SJIS������p).
  *
  * @param filename �����t�@�C���̃p�X(SJIS������).
  *
  * @return �����I�u�W�F�N�g�ւ̃|�C���^.
  *
  * �����V���b�g�Đ������Ȃ玟�̂悤�ɒ���Play()���Ăяo�����Ƃ��ł���.
  * @code
  * Audio::Engine::Instance().Prepare("�t�@�C���p�X")->Play()
  * @endcode
  *
  * ���ʉ��̂悤�ɉ��x���Đ����鉹���A�܂��͒�~�̕K�v�����鉹���̏ꍇ�A�߂�l��ϐ��Ɋi�[���Ă�����
  * �K�v�ȃ^�C�~���O�Ŋ֐����Ă�.
  * @code
  * // �������������A�߂�l�̉�������I�u�W�F�N�g��ϐ�se�Ɋi�[.
  * Audio::SoundPtr se = Audio::Engine::Instance().Prepare("�t�@�C���p�X");
  * �`�`�`
  * // �ϐ�se�Ɋi�[������������I�u�W�F�N�g���g���čĐ�.
  * se->Play();
  * @endcode
  */
  virtual SoundPtr Prepare(const char* filename) override {
    std::vector<wchar_t> wcFilename(std::strlen(filename) + 1);
    const size_t len = mbstowcs(wcFilename.data(), filename, wcFilename.size());
    if (len != static_cast<size_t>(-1)) {
      wcFilename[len] = L'\0';
      if (SoundPtr p = Prepare(wcFilename.data())) {
        return p;
      }
      if (SoundPtr p = PrepareMFStream(wcFilename.data())) {
        return p;
      }
    }
    std::cerr << "ERROR: " << filename << "��ǂݍ��߂܂���." << std::endl;
    return std::make_shared<NullSoundImpl>();
  }

  /**
  * ��������������(UTF-16������p).
  *
  * @param filename �����t�@�C���̃p�X(UTF-16������).
  *
  * @return �����I�u�W�F�N�g�ւ̃|�C���^.
  *
  * ���݂̂Ƃ���A���̊֐���Media Foundation���Ή��̂��߁A�ʏ��SJIS������ł�Prepare���g�p���邱��.
  */
  virtual SoundPtr Prepare(const wchar_t* filename) override {
    ScopedHandle hFile = CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
    if (!hFile) {
      return nullptr;
    }
    WF wf;
    std::shared_ptr<SoundImpl> sound(new SoundImpl);
    if (!LoadWaveFile(hFile, wf, sound->seekTable, &sound->source)) {
      return nullptr;
    }
    if (FAILED(xaudio->CreateSourceVoice(&sound->sourceVoice, &wf.u.ext.Format))) {
      return nullptr;
    }
    sound->engine = shared_from_this();
    soundList.push_back(sound);
    return sound;
  }

  /**
  * �X�g���[�~���O���s����������������(UTF-16������p).
  *
  * @param filename �����t�@�C���̃p�X(UTF-16������).
  *
  * @return �����I�u�W�F�N�g�ւ̃|�C���^.
  *
  * ���̊֐���XAudio2���Ή����Ă���t�@�C���`��(WAV�y��XWM)�̂ݍĐ��ł���.
  * ����ȊO�̃t�@�C���`��(MP3, AAC, WMA�Ȃ�)��PremareMFStream���g�p���邱��.
  *
  * @note PrepareMFStream��XAudio2�ɂ��Ή����Ă��邽�߁A������ɂ��̊֐����g���K�v�͂Ȃ�.
  *
  * @note �ʏ�́A���̊֐��ł͂Ȃ�Prepare�֐����g������.
  *
  * @note ���̊֐��͌���1�̉��������Đ��ł��Ȃ�.
  */
  virtual SoundPtr PrepareStream(const wchar_t* filename) override {
    streamSound.reset(new StreamSoundImpl(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr)));
    if (!streamSound->handle) {
      return nullptr;
    }
    WF wf;
    if (!LoadWaveFile(streamSound->handle, wf, streamSound->seekTable, nullptr)) {
      return nullptr;
    }
    if (FAILED(xaudio->CreateSourceVoice(&streamSound->sourceVoice, &wf.u.ext.Format))) {
      return nullptr;
    }
    streamSound->dataOffset = wf.dataOffset;
    streamSound->dataSize = wf.dataSize;
    streamSound->packetSize = wf.u.ext.Format.nBlockAlign;
    streamSound->packedAlignedBufferSize = (StreamSoundImpl::BUFFER_SIZE / streamSound->packetSize) * streamSound->packetSize;
    streamSound->lastSeekValue = 0;
    streamSound->engine = shared_from_this();
    return streamSound;
  }

  /**
  * �X�g���[�~���O���s����������������(UTF-16������p).
  *
  * @param filename �����t�@�C���̃p�X(UTF-16������).
  *
  * @return �����I�u�W�F�N�g�ւ̃|�C���^.
  *
  * Windows��Media Foundation�𗘗p����AAC��MP3�Ȃǂ̍Đ����\�ɂ��Ă���.
  * PrepareStream�Ƃ͈قȂ�A���������̓����Đ����s�����Ƃ��ł���.
  *
  * @note �ʏ�́A���̊֐��ł͂Ȃ�Prepare�֐����g������.
  */
  virtual SoundPtr PrepareMFStream(const wchar_t* filename) override {
    std::shared_ptr<MFStreamSoundImpl> mfs = std::make_shared<MFStreamSoundImpl>();
    if (!mfs->Init(xaudio, attributes.Get(), filename)) {
      return nullptr;
    }
    mfs->engine = shared_from_this();
    mfSoundList.push_back(mfs);
    return mfs;
  }

  /**
  * �}�X�^�[�{�����[����ݒ肷��.
  *
  * @param vol �ݒ肷�鉹��.
  */
  virtual void SetMasterVolume(float vol) override {
    if (xaudio) {
      masteringVoice->SetVolume(vol);
    }
  }

  /**
  * �}�X�^�[�{�����[�����擾����.
  *
  * @return �ݒ肳��Ă��鉹��.
  */
  virtual float GetMasterVolume() const override {
    if (xaudio) {
      float vol;
      masteringVoice->GetVolume(&vol);
      return vol;
    }
    return 0;
  }

private:
  ComPtr<IXAudio2> xaudio;
  IXAudio2MasteringVoice* masteringVoice = nullptr;

  typedef std::list<std::shared_ptr<SoundImpl>> SoundList;
  SoundList soundList;
  std::shared_ptr<StreamSoundImpl> streamSound;

  std::unique_ptr<MediaFoundationInitialize> mf;
  ComPtr<IMFByteStream> pMFByteStream;
  ComPtr<IMFSourceReader> pMFSourceReader;
  ComPtr<IMFAttributes> attributes;
  typedef std::list<std::shared_ptr<MFStreamSoundImpl>> MFSoundList;
  MFSoundList mfSoundList;
};

/**
* �I�[�f�B�I�G���W���̃V���O���g���C���X�^���X���擾����.
*
* @return �I�[�f�B�I�G���W���̃V���O���g���C���X�^���X�ւ̎Q��.
*/
Engine& Engine::Instance()
{
  static std::shared_ptr<EngineImpl> engine = std::make_shared<EngineImpl>();
  return *engine;
}

} // namespace Audio
