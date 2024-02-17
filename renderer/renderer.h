#pragma once
#include <d3d12.h>

namespace Renderer
{
	class BaseRenderer
	{
	public:
		BaseRenderer();
		virtual ~BaseRenderer();
		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight);
		void Update(float delta);
	protected:
		virtual void FirstFrame(class ID3D12GraphicsCommandList* InCmd);
		virtual void PreRender();
		virtual void PostRender();
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
	};
}