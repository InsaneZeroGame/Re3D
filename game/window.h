#pragma once
#include <GLFW/glfw3.h>

namespace Window
{
	class BaseWindow
	{
	public:
		explicit BaseWindow(int InWidth, int InHeight);
		virtual ~BaseWindow();
		void OnResize(int InWidth, int InHeight);
	private:
		int mWidth;
		int mHeight;
		GLFWwindow* mWindow;

	};
	inline static BaseWindow* gMainWindow = nullptr;
}