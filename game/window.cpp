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
	mWindow = glfwCreateWindow(800, 600, "Hello World", NULL, NULL);
	if (!mWindow)
	{
		glfwTerminate();
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwSetWindowSizeCallback(mWindow, &sOnResize);
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(mWindow))
	{
		/* Render here */

		/* Swap front and back buffers */
		glfwSwapBuffers(mWindow);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
}

Window::BaseWindow::~BaseWindow()
{
	glfwDestroyWindow(mWindow);
}

void Window::BaseWindow::OnResize(int InWidth, int InHeight)
{

}
