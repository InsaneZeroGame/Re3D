#include <iostream>
#include "window.h"
#include <renderer.h>
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;


int main(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	//1.Renderer
	auto renderer = new Renderer::BaseRenderer;
	//2.Window
	Window::gMainWindow = new Window::BaseWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	//3.Bind renderer and window
	renderer->SetTargetWindowAndCreateSwapChain((HWND)Window::gMainWindow->GetNativeWindow(), Window::gMainWindow->GetWidth(), Window::gMainWindow->GetHeight());
	Window::gMainWindow->SetRenderFunc(std::bind(&Renderer::BaseRenderer::Update,renderer,std::placeholders::_1));
	//4.Window Message Loop
	Window::gMainWindow->WindowLoop();
	delete Window::gMainWindow;
	return 0;
}