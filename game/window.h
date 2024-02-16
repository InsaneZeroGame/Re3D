#pragma once
#include <GLFW/glfw3.h>
#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // WIN32


namespace Window
{
	class BaseWindow
	{
	public:
		explicit BaseWindow(int InWidth, int InHeight);
		virtual ~BaseWindow();
		void OnResize(int InWidth, int InHeight);
		void* GetNativeWindow();
		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; };
		void WindowLoop();
		void SetRenderFunc(std::function<void(float)> InFunc);
	private:
		int mWidth;
		int mHeight;
		GLFWwindow* mWindow;
		std::function<void(float)> mRenderFunc;
	};
	inline static BaseWindow* gMainWindow = nullptr;
}