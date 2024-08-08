#pragma once
#include "camera.h"
#include "game_scene.h"

namespace Renderer
{
	class BaseRenderer
	{
	public:
		BaseRenderer();

		virtual ~BaseRenderer();

		virtual void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight);

		virtual void LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene);

		virtual void Update(float delta);

	protected:
		int mWidth;
		int mHeight;
		HWND mWindow;
		std::unique_ptr<class DeviceManager> mDeviceManager;
		std::unique_ptr<Gameplay::PerspectCamera> mDefaultCamera;
		std::unique_ptr<Gameplay::PerspectCamera> mShadowCamera;
		D3D12_VIEWPORT mViewPort;
		D3D12_RECT mRect;
		std::shared_ptr<class CmdManager> mCmdManager;
		std::shared_ptr<class RendererContext> mContext;
		std::unique_ptr<DirectX::ResourceUploadBatch> mBatchUploader;
	};
}