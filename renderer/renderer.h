#pragma once
#include <d3d12.h>

namespace Renderer
{
	struct Vertex
	{
		std::array<float, 4> pos;
		std::array<float, 4> color;
	};

	namespace Resource 
	{
		class VertexBuffer;
		class UploadBuffer;
	}


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
		void TransitState(ID3D12GraphicsCommandList* InCmd,ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter);
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
		//std::shared_ptr<Resource::VertexBuffer> mIndexBuffer;
		std::shared_ptr<Resource::UploadBuffer> mUploadBuffer;
		ID3D12PipelineState* mPipelineState;
		ID3D12RootSignature* m_rootSignature;
	};
}