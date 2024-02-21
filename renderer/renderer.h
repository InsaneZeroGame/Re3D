#pragma once
#include <d3d12.h>
#include <asset_loader.h>
#include <camera.h>

namespace Renderer
{
	namespace Resource 
	{
		class VertexBuffer;
		class UploadBuffer;
		class DepthBuffer;
	}

	struct FrameData
	{	
		DirectX::SimpleMath::Matrix mPrj;
		DirectX::SimpleMath::Matrix mView;
		DirectX::SimpleMath::Matrix mPrjView;
	};

	class BaseRenderer
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();
		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight);
		void Update(float delta);
	protected:
		void CreateBuffers();
		virtual void FirstFrame();
		virtual void PreRender();
		virtual void PostRender();
		virtual void CreatePipelineState();
		virtual void CreateRootSignature();
		virtual void RenderObject(const AssetLoader::ModelAsset& InAsset);
		void TransitState(ID3D12GraphicsCommandList* InCmd,ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter);
		D3D12_SHADER_BYTECODE ReadShader(_In_z_ const wchar_t* name);
		void UpdataFrameData();
	protected:
		std::unique_ptr<class DeviceManager> mDeviceManager;
		std::shared_ptr<class CmdManager> mCmdManager;
		bool mIsFirstFrame;
		int mCurrentBackbufferIndex;
		uint64_t mFenceValue;
		ID3D12Fence* mFrameFence;
		HANDLE mFrameDoneEvent;
		ID3D12GraphicsCommandList* mGraphicsCmd;
		std::shared_ptr<Resource::VertexBuffer> mVertexBuffer;
		std::shared_ptr<Resource::VertexBuffer> mIndexBuffer;
		std::shared_ptr<Resource::UploadBuffer> mUploadBuffer;
		//temp 
		std::shared_ptr<Resource::UploadBuffer> mIndexUploadBuffer;

		ID3D12PipelineState* mPipelineState;
		ID3D12RootSignature* m_rootSignature;
		AssetLoader::ModelAsset mCurrentModel;
		std::unique_ptr<Gameplay::PerspectCamera> mDefaultCamera;
		FrameData mFrameDataCPU;
		std::shared_ptr<Resource::UploadBuffer> mFrameDataGPU;
		std::shared_ptr<Resource::DepthBuffer> mDepthBuffer;
		void* mFrameDataPtr;
		int mWidth;
		int mHeight;
	};

}