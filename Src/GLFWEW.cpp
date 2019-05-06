/**
* @file GLFWE.cpp
*/
#include "GLFWEW.h"
#include <iostream>

/// GLFWとGLEWをラップするための名前空間.
namespace GLFWEW {

/**
* OpenGLからのエラー報告を処理する.
*/
void APIENTRY OutputGLDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam)
{
  std::cerr << message << "\n";
}

/**
* GLFWからのエラー報告を処理する.
*
* @param error エラー番号.
* @param desc  エラーの内容.
*/
void ErrorCallback(int error, const char* desc)
{
  std::cerr << "ERROR: " << desc << std::endl;
}

/**
* シングルトンインスタンスを取得する.
*
* @return Windowのシングルトンインスタンス.
*/
Window& Window::Instance()
{
  static Window instance;
  return instance;
}

/**
* デストラクタ.
*/
Window::~Window()
{
  if (isGLFWInitialized) {
    glfwTerminate();
  }
}

/**
* GLFW/GLEWの初期化.
*
* @param w ウィンドウの描画範囲の幅(ピクセル).
* @param h ウィンドウの描画範囲の高さ(ピクセル).
* @param title ウィンドウタイトル(UTF-8の0終端文字列).
*
* @retval true 初期化成功.
* @retval false 初期化失敗.
*/
bool Window::Init(int w, int h, const char* title)
{
  if (isInitialized) {
    std::cerr << "ERROR: GLFWEWは既に初期化されています." << std::endl;
    return false;
  }
  if (!isGLFWInitialized) {
    glfwSetErrorCallback(ErrorCallback);
    if (glfwInit() != GL_TRUE) {
      return false;
    }
    isGLFWInitialized = true;
  }

  if (!window) {
    window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
      return false;
    }
    glfwMakeContextCurrent(window);
  }

  if (glewInit() != GLEW_OK) {
    std::cerr << "ERROR: GLEWの初期化に失敗しました." << std::endl;
    return false;
  }

  for (auto& e : keyState) {
    e = KeyState::release;
  }

  width = w;
  height = h;
  glDebugMessageCallback(OutputGLDebugMessage, nullptr);

  const GLubyte* renderer = glGetString(GL_RENDERER);
  std::cout << "Renderer: " << renderer << "\n";
  const GLubyte* version = glGetString(GL_VERSION);
  std::cout << "Version: " << version << "\n";
  GLint offsetAlignment = 0;
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &offsetAlignment);
  std::cout << "GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT: " << offsetAlignment << " Bytes\n";
  GLint maxVertexUniformComponents = 0;
  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
  std::cout << "GL_MAX_VERTEX_UNIFORM_COMPONENTS: " << maxVertexUniformComponents / 1024 << " KBytes\n";
  GLint maxFragmentUniformComponents = 0;
  glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniformComponents);
  std::cout << "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS: " << maxFragmentUniformComponents / 1024 << " KBytes\n";
  GLint maxUniformBlockSize = 0;
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
  std::cout << "GL_MAX_UNIFORM_BLOCK_SIZE: " << maxUniformBlockSize / 1024 / 1024 << " MBytes\n";

  const char* glfwVersion = glfwGetVersionString();
  std::cout << "GLFW Version: " << glfwVersion << "\n";
  const char* glewVersion = reinterpret_cast<const char*>(glewGetString(GLEW_VERSION));
  std::cout << "GLEW Version: " << glewVersion << "\n";

  ResetDeltaTime();

  isInitialized = true;
  return true;
}

/**
*
*/
void Window::Update()
{
  // キー状態の更新.
  for (size_t i = 0; i < keyState.size(); ++i) {
    const bool pressed = glfwGetKey(window, i) == GLFW_PRESS;
    if (pressed) {
      if (keyState[i] == KeyState::release) {
        keyState[i] = KeyState::startPress;
      } else if (keyState[i] == KeyState::startPress) {
        keyState[i] = KeyState::press;
      }
    } else if (keyState[i] != KeyState::release) {
      keyState[i] = KeyState::release;
    }
  }

  UpdateGamePad();

  const double t = glfwGetTime();
  deltaTime = t - prevTime;
  if (deltaTime >= 0.5) {
    deltaTime = 1.0 / 60.0;
  }
  prevTime = t;
}

/**
*
*/
void Window::ResetDeltaTime()
{
  prevTime = glfwGetTime();
  deltaTime = 0;
}

/**
* キーが押された瞬間か調べる.
*
* @param key 調べるキーのID(GLFW_KEY_Aなど).
*
* @retval true  キーが押された瞬間.
* @retval false キーが押された瞬間ではない.
*/
bool Window::KeyDown(int key) const
{
  if (key < 0 || key >= static_cast<int>(keyState.size())) {
    return false;
  }
  return keyState[key] == KeyState::startPress;
}

/**
* キーが押されているか調べる.
*
* @param key 調べるキーのID(GLFW_KEY_Aなど).
*
* @retval true  キーが押されている.
* @retval false キーが押されていない.
*/
bool Window::KeyPressed(int key) const
{
  if (key < 0 || key >= static_cast<int>(keyState.size())) {
    return false;
  }
  return keyState[key] != KeyState::release;
}

/**
* ウィンドウを閉じるべきか調べる.
*
* @retval true 閉じる.
* @retval false 閉じない.
*/
bool Window::ShouldClose() const
{
  return glfwWindowShouldClose(window) != 0;
}

/**
* フロントバッファとバックバッファを切り替える.
*/
void Window::SwapBuffers() const
{
  glfwPollEvents();
  glfwSwapBuffers(window);
}

/**
* マウスの座標を取得する.
*
* @return ウィンドウの左上隅を(0,0)としたマウスの座標.
*/
glm::vec2 Window::MousePosition() const
{
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  return glm::vec2(x, y);
}

/**
* マウスカーソルを非表示にする.
*
* カーソル移動情報は、引き続き取得可能.
*/
void Window::DisableMouseCursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

/**
* マウスカーソルを表示する.
*/
void Window::EnableMouseCursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

/**
* ジョイスティックのアナログ入力装置ID.
+
* @note XBOX360コントローラー基準.
*/
enum JOYSTICK_AXES {
  JOYSTICK_AXES_LEFT_X,  ///< 左スティックのX軸.
  JOYSTICK_AXES_LEFT_Y,  ///< 左スティックのY軸.
  JOYSTICK_AXES_TRIGGER, ///< アナログトリガー.
  JOYSTICK_AXES_RIGHT_Y, ///< 右スティックのY軸.
  JOYSTICK_AXES_RIGHT_X, ///< 右スティックのX軸.
};

/**
* ジョイスティックのデジタル入力装置ID.
*
* @note Xbox Oneコントローラー準拠.
*/
enum JOYSTICK_BUTTON {
  JOYSTICK_BUTTON_A, ///< Aボタン.
  JOYSTICK_BUTTON_B, ///< Bボタン.
  JOYSTICK_BUTTON_X, ///< Xボタン.
  JOYSTICK_BUTTON_Y, ///< Yボタン.
  JOYSTICK_BUTTON_L, ///< Lボタン.
  JOYSTICK_BUTTON_R, ///< Rボタン.
  JOYSTICK_BUTTON_BACK, ///< Backボタン.
  JOYSTICK_BUTTON_START, ///< Startボタン.
  JOYSTICK_BUTTON_L_THUMB, ///< 左スティックボタン.
  JOYSTICK_BUTTON_R_THUMB, ///< 右スティックボタン.
  JOYSTICK_BUTTON_UP, ///< 上キー.
  JOYSTICK_BUTTON_RIGHT, ///< 右キー.
  JOYSTICK_BUTTON_DOWN, ///< 下キー.
  JOYSTICK_BUTTON_LEFT, ///< 左キー.
};

/**
* ゲームパッドの状態を更新する.
*/
void Window::UpdateGamePad()
{
  const uint32_t prevButtons = gamepad.buttons;
  int axesCount, buttonCount;
  const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);
  const uint8_t* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttonCount);
  if (axes && buttons && axesCount >= 2 && buttonCount >= 8) {
    gamepad.buttons &= ~(GamePad::DPAD_UP | GamePad::DPAD_DOWN | GamePad::DPAD_LEFT | GamePad::DPAD_RIGHT);
    static const float threshould = 0.3f;
    if (axes[JOYSTICK_AXES_LEFT_Y] >= threshould) {
      gamepad.buttons |= GamePad::DPAD_UP;
    } else if (axes[JOYSTICK_AXES_LEFT_Y] <= -threshould) {
      gamepad.buttons |= GamePad::DPAD_DOWN;
    }
    if (axes[JOYSTICK_AXES_LEFT_X] >= threshould) {
      gamepad.buttons |= GamePad::DPAD_LEFT;
    } else if (axes[JOYSTICK_AXES_LEFT_X] <= -threshould) {
      gamepad.buttons |= GamePad::DPAD_RIGHT;
    }
    static const struct {
      int glfwCode;
      uint32_t gamepadCode;
    } keyMap[] = {
      { JOYSTICK_BUTTON_A, GamePad::A },
      { JOYSTICK_BUTTON_B, GamePad::B },
      { JOYSTICK_BUTTON_X, GamePad::X },
      { JOYSTICK_BUTTON_Y, GamePad::Y },
      { JOYSTICK_BUTTON_L, GamePad::L },
      { JOYSTICK_BUTTON_R, GamePad::R },
      { JOYSTICK_BUTTON_START, GamePad::START },
    };
    for (const auto& e : keyMap) {
      if (buttons[e.glfwCode] == GLFW_PRESS) {
        gamepad.buttons |= e.gamepadCode;
      } else if (buttons[e.glfwCode] == GLFW_RELEASE) {
        gamepad.buttons &= ~e.gamepadCode;
      }
    }
  } else {
    static const struct {
      int glfwCode;
      uint32_t gamepadCode;
    } keyMap[] = {
      { GLFW_KEY_W, GamePad::DPAD_UP },
      { GLFW_KEY_A, GamePad::DPAD_DOWN },
      { GLFW_KEY_S, GamePad::DPAD_LEFT },
      { GLFW_KEY_D, GamePad::DPAD_RIGHT },
      { GLFW_KEY_ENTER, GamePad::START },
      { GLFW_KEY_J, GamePad::A },
      { GLFW_KEY_K, GamePad::B },
      { GLFW_KEY_U, GamePad::X },
      { GLFW_KEY_I, GamePad::Y },
      { GLFW_KEY_O, GamePad::L },
      { GLFW_KEY_L, GamePad::R },
    };
    for (const auto& e : keyMap) {
      const int key = glfwGetKey(window, e.glfwCode);
      if (key == GLFW_PRESS) {
        gamepad.buttons |= e.gamepadCode;
      } else if (key == GLFW_RELEASE) {
        gamepad.buttons &= ~e.gamepadCode;
      }
    }
  }
  gamepad.buttonDown = gamepad.buttons & ~prevButtons;
}

} // namespace GLFWEW
