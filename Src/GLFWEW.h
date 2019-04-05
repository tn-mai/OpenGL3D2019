/**
* @file GLFWEW.h
*/
#ifndef GLFWEW_H_INCLUDED
#define GLFWEW_H_INCLUDED

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>

namespace GLFWEW {

/**
* GLFWとGLEWのラッパークラス.
*/
class Window
{
public:
  static Window& Instance();
  bool Init(int w, int h, const char* title);
  void Update();
  bool ShouldClose() const;
  void SwapBuffers() const;
  int Width() const { return width; }
  int Height() const { return height; }
  void ResetDeltaTime();
  double DeltaTime() const { return deltaTime; }
  bool KeyDown(int key) const;
  bool KeyPressed(int key) const;

private:
  Window() = default;
  ~Window();
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool isGLFWInitialized = false;
  bool isInitialized = false;
  GLFWwindow* window = nullptr;
  int width = 0;
  int height = 0;
  double prevTime = 0;
  double deltaTime = 0;

  enum class KeyState : char {
    release,
    startPress,
    press,
  };
  std::array<KeyState, GLFW_KEY_LAST + 1> keyState = { KeyState::release };
};

} // namespace GLFWEW

#endif // GLFWEW_H_INCLUDED