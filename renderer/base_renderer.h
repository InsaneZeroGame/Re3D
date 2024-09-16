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

		std::unordered_map<std::string, std::shared_ptr<Resource::Texture>>& GetSceneTextureMap();


		//Todo: Remove this temp code for mesh shader
		virtual void MeshShaderNewStaticmeshComponent(ECS::StaticMeshComponent& InStaticMeshComponent) {};

	public:
		//Tone Mapping Settings
		bool mUseToneMapping = true;
		float mExposure = 0.0f;

		//Bloom Settings
		float mBloomThreshold = 0.25;
		float mBloomBlurKernelSize = 4.0f;
		float mBloomBrightness = 1.0f;
		float mbloomIntensity = 0.0f;
		float mbaseIntensity = 1.0f;
		float mbloomSaturation = 1.0f;
		float mbloomBaseSaturation = 1.0f;
		std::array<float, 3> mSunLightDir = { 1.0,1.0,1.0 };
		float mSunLightIntensity = 1.0;
		virtual void CreateBuffers();
		virtual void UpdataFrameData();
		virtual void PrepairForRendering();
		virtual void FirstFrame();
		virtual std::shared_ptr<class RendererContext> GetContext() { return nullptr; };
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
		std::unique_ptr<DirectX::ResourceUploadBatch> mBatchUploader;
		uint64_t mGraphicsFenceValue;
		std::shared_ptr<GAS::GameScene> mCurrentScene;
		std::future<void> mLoadResourceFuture;
		std::unordered_map<std::string, std::shared_ptr<Resource::Texture>> mTextureMap;
		std::shared_ptr<class Gui> mGui;
		std::array<FrameData, SWAP_CHAIN_BUFFER_COUNT> mFrameData;
		std::array<std::shared_ptr<Resource::UploadBuffer>, SWAP_CHAIN_BUFFER_COUNT> mFrameDataCPU;
		std::array<std::unique_ptr<Resource::VertexBuffer>, SWAP_CHAIN_BUFFER_COUNT> mFrameDataGPU;
		std::unique_ptr<Resource::StructuredBuffer> mLightBuffer;
		std::shared_ptr<Resource::UploadBuffer> mLightUploadBuffer;
		int mFrameIndexCpu = 0;
		std::array<ECS::LigthData, 256> mLights;
		std::mutex mLoadResourceMutex;
	};
}