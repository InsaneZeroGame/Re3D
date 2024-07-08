#include <iostream>
#include "window.h"
#include <renderer.h>
#include "asset_loader.h"
#include <memory>
#include "game_scene.h"
constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/,
                    _In_ int nCmdShow) {
	//asset loader
	AssetLoader::InitAssetLoader();
	//Game Scene 
	std::shared_ptr<GAS::GameScene> newScene = std::make_shared<GAS::GameScene>();
    //newScene->CreateEntitiesWithMesh("SM_Shaderball.FBX");
	//newScene->CreateEntitiesWithMesh("scene.fbx");

	//1.Renderer
	std::shared_ptr<Renderer::BaseRenderer> renderer = std::make_shared<Renderer::BaseRenderer>();
	//2.Window
    Window::gMainWindow = new Window::Win32NavtiveWindow(WINDOW_WIDTH, WINDOW_HEIGHT, hInstance, nCmdShow);
	//3.Bind renderer and window
	renderer->SetTargetWindowAndCreateSwapChain((HWND)Window::gMainWindow->GetNativeWindow(), Window::gMainWindow->GetWidth(), Window::gMainWindow->GetHeight());
    renderer->LoadGameScene(newScene);
	Window::gMainWindow->SetRenderFunc(std::bind(&Renderer::BaseRenderer::Update,renderer.get(), std::placeholders::_1));
	//4.Window Message Loop
	Window::gMainWindow->WindowLoop();
	delete Window::gMainWindow;
	AssetLoader::DestroyAssetLoader();
	return 0;
}