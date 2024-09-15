#pragma once
#include "skybox.h"
#include "light_cull_pass.h"
#include <camera.h>
#include "game_scene.h"
#include "base_renderer.h"


namespace tf
{
	class Taskflow;
	class Executor;
}

namespace DirectX
{
	class ResourceUploadBatch;
	namespace DX12
	{
		class BasicPostProcess;
		class DualPostProcess;
		class ToneMapPostProcess;
		class GraphicsMemory;
	}
}

namespace Renderer
{
	class ClusterForwardRenderer : public std::enable_shared_from_this<ClusterForwardRenderer>,public BaseRenderer
	{
	public:
		ClusterForwardRenderer();
		virtual ~ClusterForwardRenderer();
		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight) override;
		void Update(float delta) override;
		void LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene) override;
		
	protected:
        //void CreateGui();
		void CreateRenderTask();
		void CreateBuffers() override;
		void CreateTextures();
		void DepthOnlyPass(const ECS::StaticMeshComponent& InAsset);
		void InitPostProcess();
		virtual void FirstFrame();
		virtual void PreRender();
		virtual void PostRender();
		virtual void CreatePipelineState();
		virtual void CreateRootSignature();
		void UpdataFrameData() override;
		void OnGameSceneUpdated(std::shared_ptr<GAS::GameScene> InScene, std::span<entt::entity> InNewEntities);
		virtual void DrawObject(const ECS::StaticMeshComponent& InAsset);
		void PrepairForRendering() override;
	protected:
		bool mIsFirstFrame;
		uint64_t mComputeFenceValue;
		HANDLE mComputeFenceHandle;
		ID3D12Fence* mComputeFence;
		ID3D12GraphicsCommandList* mComputeCmd;
		ID3D12GraphicsCommandList* mGraphicsCmd;
		ID3D12CommandAllocator* mGraphicsCmdAllocator;
		ID3D12PipelineState* mColorPassPipelineState;
		ID3D12PipelineState* mColorPassPipelineState8XMSAA;
		ID3D12PipelineState* mPipelineStateDepthOnly;
		ID3D12PipelineState* mPipelineStateShadowMap;
		ID3D12RootSignature* mColorPassRootSignature;
		
		
		std::unique_ptr<class tf::Taskflow> mRenderFlow;
		std::unique_ptr<class tf::Executor> mRenderExecution;
		
		float mColorRGBA[4] = { 0.15f,0.25f,0.75f,1.0f };
		
		std::unique_ptr<Resource::StructuredBuffer> mClusterBuffer;
		std::vector<Cluster> mCLusters;
		std::unique_ptr<SkyboxPass> mSkyboxPass;
		std::unique_ptr<LightCullPass> mLightCullPass;

        bool mHasSkybox = true;

		std::shared_ptr<Resource::StructuredBuffer> mDummyBuffer;
		std::unique_ptr<DirectX::DX12::BasicPostProcess> ppBloomExtract;
		std::unique_ptr<DirectX::DX12::BasicPostProcess> ppBloomBlur;
		std::unique_ptr<DirectX::DX12::DualPostProcess> ppBloomCombine;
		std::unique_ptr<DirectX::DX12::ToneMapPostProcess> ppToneMap;
		std::unique_ptr<DirectX::DX12::BasicPostProcess> mImageBlit;
		std::unique_ptr<DirectX::DX12::GraphicsMemory> mGPUMemory;
		
	};

	

}