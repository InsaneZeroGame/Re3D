#pragma once
#include "camera.h"
#include "game_scene.h"
#include <asset_loader.h>

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

		void TransitState(ID3D12GraphicsCommandList* InCmd, ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter, UINT InSubResource = 0);
		
		std::shared_ptr<Resource::Texture> LoadMaterial(std::string_view InTextureName, std::string_view InMatName = {}, const std::wstring& InDebugName = L"");
		
		std::shared_ptr<Resource::Texture> LoadMaterial(std::string_view InTextureName, AssetLoader::TextureData* textureData, const std::wstring& InDebugName = L"");

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
		uint64_t mGraphicsFenceValue;
		std::shared_ptr<GAS::GameScene> mCurrentScene;
		std::future<void> mLoadResourceFuture;
		std::unordered_map<std::string, std::shared_ptr<Resource::Texture>> mTextureMap;

	};
}