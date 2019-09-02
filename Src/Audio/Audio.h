/**
* @file Audio.h
*/
#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED
#include <memory>

namespace Audio {

/**
* 音声の再生状態.
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
* 音声の再生制御フラグ.
*/
enum Flag
{
	Flag_None = 0,
	Flag_Loop = 0x01,
};

/**
* 音声制御インターフェイス.
*
* Engine::Prepare, Engine::PrepareStream関数を使って取得する.
*/
class Sound
{
public:
	virtual ~Sound() = default;
	virtual bool Play(int flags = 0) = 0; ///< 再生及び一時停止中の再開.
	virtual bool Pause() = 0; ///< 一時停止.
	virtual bool Seek() = 0; ///< 再生位置の変更(未実装).
	virtual bool Stop() = 0; ///< 停止.
  virtual float SetVolume(float) = 0; ///< 音量の設定(既定値=1.0).
	virtual float SetPitch(float) = 0; ///< 音程の設定(既定値=1.0);
	virtual int GetState() const = 0; ///< 再生状態の取得.
  virtual float GetVolume() const = 0; ///< 音量の取得.
  virtual float GetPitch() const = 0; ///< 音程の取得.
  virtual bool IsNull() const = 0; ///< 音声設定の有無(true=設定されていない, false=設定されている)
};
typedef std::shared_ptr<Sound> SoundPtr;

/**
* 音声再生の制御クラス.
*
* 初期化と終了:
* -# アプリケーションの初期化処理でEngine::Initialize()を呼び出す.
* -# アプリケーションの終了処理でEngine::Finalize()を呼び出す.
*
* 音声の再生:
* -# 音声ファイル名を引数にしてPrepare(ぷりぺあ)関数を呼び出すと、戻り値として「音声制御インターフェイス」が返されるので、これを変数に保存する.
* -# 音声制御インターフェイスに対してPlay関数を呼び出すと音声が再生される. Playを呼び出すたびに同じ音声が再生される.
* -# Playを呼び出す必要がなくなったら音声制御インターフェイスを破棄する.
* -# 使いまわしをしない音声の場合「engine.Prepare("OneTimeSound.wav")->Play()」のように書くことができる.
*/
class Engine
{
public:
  static Engine& Instance();

  Engine() = default;

  virtual ~Engine() = default;
  virtual bool Initialize() = 0; ///< 初期化.
  virtual void Finalize() = 0; ///< 破棄.
  virtual bool Update() = 0; ///< 更新.
  virtual SoundPtr Prepare(const char*) = 0; ///< ファイル名からSE用音声制御インターフェイスを得る.
  virtual SoundPtr Prepare(const wchar_t*) = 0; ///< ファイル名からSE用音声制御インターフェイスを得る.
  virtual void SetMasterVolume(float) = 0; ///< 全体音量の設定(既定値=1.0).
  virtual float GetMasterVolume() const = 0; ///< 全体音量の取得.

  virtual SoundPtr PrepareStream(const wchar_t*) = 0; ///< ファイル名からBGM用音声制御インターフェイスを得る.
  virtual SoundPtr PrepareMFStream(const wchar_t*) = 0; ///< ファイル名からBGM用音声制御インターフェイスを得る.

private:
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
};

} // namespace Audio

#endif // AUDIO_H_INCLUDED