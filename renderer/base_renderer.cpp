#include "base_renderer.h"

Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mContext(std::make_shared<RendererContext>(mCmdManager)),
	mBatchUploader(std::make_unique<ResourceUploadBatch>(g_Device))
{

}

Renderer::BaseRenderer::~BaseRenderer()
{

}

void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	mWindow = InWindow;
	mWidth = InWidth;
	mHeight = InHeight;
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	//Use Reverse Z
	mDefaultCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f, true);
	mDefaultCamera->LookAt({ 3.0,3.0,2.0 }, { 0.0f,3.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mShadowCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f, false);
	mShadowCamera->LookAt({ 5,25,0.0 }, { 0.0f,0.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mViewPort = { 0,0,(float)mWidth,(float)mHeight,0.0,1.0 };
	mRect = { 0,0,mWidth,mHeight };
}

void Renderer::BaseRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{

}

void Renderer::BaseRenderer::Update(float delta)
{

}
