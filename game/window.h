#pragma once
#pragma warning(disable : 4996)
#include <GLFW/glfw3.h>
#ifdef WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>
#endif // WIN32
#include <delegates.h>

namespace Window
{
	class BaseWindow
	{
	public:
		BaseWindow(int InWidth, int InHeight);
		virtual ~BaseWindow();
		virtual void OnResize(int InWidth, int InHeight) = 0;
		virtual void* GetNativeWindow() = 0;
		int GetWidth();
		int GetHeight();
		virtual void WindowLoop() = 0;
		void SetRenderFunc(std::function<void(float)> InFunc);
	protected:
		int mWidth;
		int mHeight;
		std::function<void(float)> mRenderFunc;

	};

	class Win32NavtiveWindow : public BaseWindow{
    public:
        Win32NavtiveWindow(int InWidth, int InHeight,HINSTANCE hInst, int nCmdShow);
        ~Win32NavtiveWindow();
        // Inherited via BaseWindow
        void OnResize(int InWidth, int InHeight) override;
        void* GetNativeWindow() override;
        void WindowLoop() override;

    private:
        HWND mNativeWindow;
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        std::unique_ptr<DirectX::Keyboard> mKeyboard; 
    };


	class GlfwWindow : public BaseWindow
	{
	public:
		explicit GlfwWindow(int InWidth, int InHeight);
		virtual ~GlfwWindow();

		void OnResize(int InWidth, int InHeight) override;

		void* GetNativeWindow() override;

		void WindowLoop() override;

	private:
		
		GLFWwindow* mWindow;
	};
	//inline GlfwWindow* gMainWindow = nullptr;
    inline Win32NavtiveWindow* gMainWindow = nullptr;
    }