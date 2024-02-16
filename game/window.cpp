#include "window.h"

namespace Window
{
	static void sOnResize(GLFWwindow* window, int InWidth, int InHeight)
	{
		gMainWindow->OnResize(InWidth, InHeight);
	}
}

Window::BaseWindow::BaseWindow(int InWidth, int InHeight):
	mWidth(InWidth),
	mHeight(InHeight)
{
	/* Initialize the library */
	Ensures(glfwInit());

	/* Create a windowed mode window and its OpenGL context */
	mWindow = glfwCreateWindow(InWidth, InHeight, "Re3D", NULL, NULL);
	if (!mWindow)
	{
		glfwTerminate();
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwSetWindowSizeCallback(mWindow, &sOnResize);
}

Window::BaseWindow::~BaseWindow()
{
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void Window::BaseWindow::OnResize(int InWidth, int InHeight)
{
	std::cout << "Width : " << InWidth << " Height : " << InHeight << std::endl;
}

void* Window::BaseWindow::GetNativeWindow()
{
	return glfwGetWin32Window(mWindow);
}

void Window::BaseWindow::WindowLoop()
{
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(mWindow))
	{
		/* Render here */
		if (mRenderFunc)
		{
			mRenderFunc(0.0f);
		}
		/* Swap front and back buffers */
		glfwSwapBuffers(mWindow);

		/* Poll for and process events */
		glfwPollEvents();
	}
}

void Window::BaseWindow::SetRenderFunc(std::function<void(float)> InFunc)
{
	mRenderFunc = InFunc;
}
