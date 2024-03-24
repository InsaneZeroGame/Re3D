#include <iostream>
#include "window.h"
#include <renderer.h>
#include "asset_loader.h"
#include <memory>
#include "game_scene.h"
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;


int main(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	//asset loader
	AssetLoader::InitAssetLoader();
	//Game Scene 
	std::shared_ptr<GAS::GameScene> newScene = std::make_shared<GAS::GameScene>();
	newScene->CreateEntityWithMesh("scene.obj");

	//1.Renderer
	std::unique_ptr<Renderer::BaseRenderer> renderer = std::make_unique<Renderer::BaseRenderer>();
	renderer->LoadGameScene(newScene);
	//2.Window
	Window::gMainWindow = new Window::BaseWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	//3.Bind renderer and window
	renderer->SetTargetWindowAndCreateSwapChain((HWND)Window::gMainWindow->GetNativeWindow(), Window::gMainWindow->GetWidth(), Window::gMainWindow->GetHeight());
	Window::gMainWindow->SetRenderFunc(std::bind(&Renderer::BaseRenderer::Update,renderer.get(), std::placeholders::_1));
	//4.Window Message Loop
	Window::gMainWindow->WindowLoop();
	delete Window::gMainWindow;
	AssetLoader::DestroyAssetLoader();
	return 0;
}