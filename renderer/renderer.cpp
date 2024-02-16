#include "renderer.h"
#include "device_manager.h"


Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>())
{

}

Renderer::BaseRenderer::~BaseRenderer()
{

}

void Renderer::BaseRenderer::SetTargetWindow(HWND InWindow, int InWidth, int InHeight)
{
	mDeviceManager->SetTargetWindow(InWindow, InWidth, InHeight);
}
