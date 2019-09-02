/**
* @file Audio.h
*/
#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED
#include <memory>

namespace Audio {

/**
* �����̍Đ����.
*/
enum State {
	State_Create = 0x01,
	State_Preparing = 0x02,
	State_Prepared = 0x04,
	State_Playing = 0x08,
	State_Stopped = 0x10,
	State_Pausing = 0x20,
	State_Failed = 0x40,
};

/**
* �����̍Đ�����t���O.
*/
enum Flag
{
	Flag_None = 0,
	Flag_Loop = 0x01,
};

/**
* ��������C���^�[�t�F�C�X.
*
* Engine::Prepare, Engine::PrepareStream�֐����g���Ď擾����.
*/
class Sound
{
public:
	virtual ~Sound() = default;
	virtual bool Play(int flags = 0) = 0; ///< �Đ��y�шꎞ��~���̍ĊJ.
	virtual bool Pause() = 0; ///< �ꎞ��~.
	virtual bool Seek() = 0; ///< �Đ��ʒu�̕ύX(������).
	virtual bool Stop() = 0; ///< ��~.
  virtual float SetVolume(float) = 0; ///< ���ʂ̐ݒ�(����l=1.0).
	virtual float SetPitch(float) = 0; ///< �����̐ݒ�(����l=1.0);
	virtual int GetState() const = 0; ///< �Đ���Ԃ̎擾.
  virtual float GetVolume() const = 0; ///< ���ʂ̎擾.
  virtual float GetPitch() const = 0; ///< �����̎擾.
  virtual bool IsNull() const = 0; ///< �����ݒ�̗L��(true=�ݒ肳��Ă��Ȃ�, false=�ݒ肳��Ă���)
};
typedef std::shared_ptr<Sound> SoundPtr;

/**
* �����Đ��̐���N���X.
*
* �������ƏI��:
* -# �A�v���P�[�V�����̏�����������Engine::Initialize()���Ăяo��.
* -# �A�v���P�[�V�����̏I��������Engine::Finalize()���Ăяo��.
*
* �����̍Đ�:
* -# �����t�@�C�����������ɂ���Prepare(�Ղ�؂�)�֐����Ăяo���ƁA�߂�l�Ƃ��āu��������C���^�[�t�F�C�X�v���Ԃ����̂ŁA�����ϐ��ɕۑ�����.
* -# ��������C���^�[�t�F�C�X�ɑ΂���Play�֐����Ăяo���Ɖ������Đ������. Play���Ăяo�����тɓ����������Đ������.
* -# Play���Ăяo���K�v���Ȃ��Ȃ����特������C���^�[�t�F�C�X��j������.
* -# �g���܂킵�����Ȃ������̏ꍇ�uengine.Prepare("OneTimeSound.wav")->Play()�v�̂悤�ɏ������Ƃ��ł���.
*/
class Engine
{
public:
  static Engine& Instance();

  Engine() = default;

  virtual ~Engine() = default;
  virtual bool Initialize() = 0; ///< ������.
  virtual void Finalize() = 0; ///< �j��.
  virtual bool Update() = 0; ///< �X�V.
  virtual SoundPtr Prepare(const char*) = 0; ///< �t�@�C��������SE�p��������C���^�[�t�F�C�X�𓾂�.
  virtual SoundPtr Prepare(const wchar_t*) = 0; ///< �t�@�C��������SE�p��������C���^�[�t�F�C�X�𓾂�.
  virtual void SetMasterVolume(float) = 0; ///< �S�̉��ʂ̐ݒ�(����l=1.0).
  virtual float GetMasterVolume() const = 0; ///< �S�̉��ʂ̎擾.

  virtual SoundPtr PrepareStream(const wchar_t*) = 0; ///< �t�@�C��������BGM�p��������C���^�[�t�F�C�X�𓾂�.
  virtual SoundPtr PrepareMFStream(const wchar_t*) = 0; ///< �t�@�C��������BGM�p��������C���^�[�t�F�C�X�𓾂�.

private:
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

} // namespace Audio

#endif // AUDIO_H_INCLUDED