#include <iostream>
#include "window.h"
#include <renderer.h>
#include <renderer_dxr.h>
#include <memory>
#include "game_scene.h"
#include "engine.h"

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;

#define USE_DXR_RENDERER


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/,
                    _In_ int nCmdShow) {

	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	engine::InitGameEngine();

	//Game Scene 
	std::shared_ptr<GAS::GameScene> newScene = std::make_shared<GAS::GameScene>();
	//1.Renderer
#ifdef USE_DXR_RENDERER
	std::shared_ptr<Renderer::DXRRenderer> renderer = std::make_shared<Renderer::DXRRenderer>();
#else
	std::shared_ptr<Renderer::ClusterForwardRenderer> renderer = std::make_shared<Renderer::ClusterForwardRenderer>();
#endif // USE_DXR_RENDERER


	//2.Window
    Window::gMainWindow = new Window::Win32NavtiveWindow(WINDOW_WIDTH, WINDOW_HEIGHT, hInstance, nCmdShow);
	//3.Bind renderer and window
	renderer->SetTargetWindowAndCreateSwapChain((HWND)Window::gMainWindow->GetNativeWindow(), Window::gMainWindow->GetWidth(), Window::gMainWindow->GetHeight());
    renderer->LoadGameScene(newScene);
#ifdef USE_DXR_RENDERER
	Window::gMainWindow->SetRenderFunc(std::bind(&Renderer::DXRRenderer::Update, renderer.get(), std::placeholders::_1));
#else
	Window::gMainWindow->SetRenderFunc(std::bind(&Renderer::ClusterForwardRenderer::Update, renderer.get(), std::placeholders::_1));
#endif
	//4.Window Message Loop
	Window::gMainWindow->WindowLoop();
	delete Window::gMainWindow;
	AssetLoader::DestroyAssetLoader();
	return 0;
}