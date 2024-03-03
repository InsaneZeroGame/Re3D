#include "window.h"

namespace Window
{
	static void sOnResize(GLFWwindow* window, int InWidth, int InHeight)
	{
		gMainWindow->OnResize(InWidth, InHeight);
	}

	static void sCursorPos(GLFWwindow* window, double x, double y)
	{

	}
	static void sOnMouseEntered(GLFWwindow* window, int Entered)
	{
		if (Entered == GLFW_TRUE)
		{
			//spdlog::info("Entered");
		}
		else
		{
			//spdlog::info("Leave");
		}
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{

		}
	}

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{

	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		for (auto& Delegate : Delegates::KeyPressDelegate)
		{
			Delegate(key, scancode, action, mods);
		}
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
	glfwMakeContextCurrent(mWindow);

	if (!mWindow)
	{
		glfwTerminate();
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwSetWindowSizeCallback(mWindow, &sOnResize);
	glfwSetCursorPosCallback(mWindow, &sCursorPos);
	glfwSetCursorEnterCallback(mWindow, &sOnMouseEntered);
	glfwSetScrollCallback(mWindow, &scroll_callback);
	glfwSetKeyCallback(mWindow, &key_callback);
	glfwSwapInterval(0);
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
