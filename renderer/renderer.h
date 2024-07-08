#pragma once
#include "skybox.h"
#include "components.h"
#include <d3d12.h>
#include <asset_loader.h>
#include <camera.h>
#include "game_scene.h"
#include <future>

namespace tf
{
	class Taskflow;
	class Executor;
}

namespace DirectX
{
	class ResourceUploadBatch;
}

namespace Renderer
{
	namespace Resource
	{
		class Texture;
		class UploadBuffer;
		class StructuredBuffer;
	}

	struct FrameData
	{	
		DirectX::SimpleMath::Matrix PrjView;
		DirectX::SimpleMath::Matrix View;
		DirectX::SimpleMath::Matrix Prj;
		DirectX::SimpleMath::Matrix NormalMatrix;
		DirectX::SimpleMath::Matrix ShadowViewMatrix;
		DirectX::SimpleMath::Matrix ShadowViewPrjMatrix;
		DirectX::SimpleMath::Vector4 DirectionalLightDir;
		DirectX::SimpleMath::Vector4 DirectionalLightColor;
	};

	struct LightCullViewData
	{
		DirectX::SimpleMath::Vector4 ViewSizeAndInvSize;
		DirectX::SimpleMath::Matrix  ClipToView;
		DirectX::SimpleMath::Matrix  ViewMatrix;
		DirectX::SimpleMath::Vector4 LightGridZParams;
		DirectX::SimpleMath::Vector4 InvDeviceZToWorldZTransform;
	};
	


	class BaseRenderer : public std::enable_shared_from_this<BaseRenderer>
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();
		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight);
		void Update(float delta);
		std::unordered_map<std::string, std::shared_ptr<Resource::Texture>>& GetSceneTextureMap();
		void LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene);
		void LoadStaticMeshToGpu(ECS::StaticMeshComponent& InComponent);
		std::shared_ptr<Resource::Texture> LoadMaterial(std::string_view InTextureName, std::string_view InMatName = {},const std::wstring& InDebugName = L"");
	protected:
        void CreateGui();
		void CreateRenderTask();
		void CreateBuffers();
		void CreateTextures();
		void DepthOnlyPass(const ECS::StaticMeshComponent& InAsset);
		void CreateSkybox();
		virtual void FirstFrame();
		virtual void PreRender();
		virtual void PostRender();
		virtual void CreatePipelineState();
		virtual void CreateRootSignature();
		virtual void RenderObject(const ECS::StaticMeshComponent& InAsset);
		void TransitState(ID3D12GraphicsCommandList* InCmd,ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter,UINT InSubResource = 0);
		void UpdataFrameData();
		
	protected:
		std::unique_ptr<class DeviceManager> mDeviceManager;
		std::shared_ptr<class CmdManager> mCmdManager;
		std::unique_ptr<class RendererContext> mContext;
		bool mIsFirstFrame;
		uint64_t mComputeFenceValue;
		uint64_t mGraphicsFenceValue;
		HANDLE mComputeFenceHandle;
		ID3D12Fence* mComputeFence;
		ID3D12GraphicsCommandList* mComputeCmd;
		ID3D12GraphicsCommandList* mGraphicsCmd;
		ID3D12CommandAllocator* mGraphicsCmdAllocator;
		ID3D12PipelineState* mColorPassPipelineState;
		ID3D12PipelineState* mPipelineStateDepthOnly;
		ID3D12PipelineState* mPipelineStateShadowMap;
		ID3D12PipelineState* mLightCullPass;
		ID3D12RootSignature* mColorPassRootSignature;
		ID3D12RootSignature* mLightCullPassRootSignature;
		std::unique_ptr<Gameplay::PerspectCamera> mDefaultCamera;
		std::unique_ptr<Gameplay::PerspectCamera> mShadowCamera;
		FrameData mFrameDataCPU;
		std::shared_ptr<Resource::UploadBuffer> mFrameDataGPU;
		int mWidth;
		int mHeight;
		std::unique_ptr<class tf::Taskflow> mRenderFlow;
		std::unique_ptr<class tf::Executor> mRenderExecution;
		D3D12_VIEWPORT mViewPort;
		D3D12_RECT mRect;
		float mColorRGBA[4] = { 0.15f,0.25f,0.75f,1.0f };
		std::unique_ptr<Resource::StructuredBuffer> mLightBuffer;
		std::shared_ptr<Resource::UploadBuffer> mLightUploadBuffer;
		std::unique_ptr<Resource::StructuredBuffer> mClusterBuffer;
		std::vector<Cluster> mCLusters;
		LightCullViewData mLightCullViewData;
		std::unique_ptr<Resource::UploadBuffer> mLightCullViewDataGpu;
		std::shared_ptr<Resource::Texture> mSkyboxTexture;
		std::unique_ptr<DirectX::ResourceUploadBatch> mBatchUploader;
		std::unique_ptr<Skybox> mSkybox;
		std::array<ECS::LightComponent, 256> mLights;
		std::shared_ptr<GAS::GameScene> mCurrentScene;
        HWND mWindow;
        std::shared_ptr<class Gui> mGui;
        bool mHasSkybox = true;
        std::future<void> mLoadResourceFuture;
		std::unordered_map<std::string, std::shared_ptr<Resource::Texture>> mTextureMap;
	};

	

}