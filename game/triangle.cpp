#include <iostream>
#include "window.h"
#include <renderer.h>
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;


int main() 
{
	std::cout << "Hello Re3D" << std::endl;
	auto renderer = new Renderer::BaseRenderer;
	Window::gMainWindow = new Window::BaseWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	renderer->SetTargetWindow((HWND)Window::gMainWindow->GetNativeWindow(), Window::gMainWindow->GetWidth(), Window::gMainWindow->GetHeight());
	Window::gMainWindow->WindowLoop();
	delete Window::gMainWindow;
	return 0;
}