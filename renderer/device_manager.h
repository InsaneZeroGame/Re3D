#pragma once
#include <queue>
#include <map>
#include "gpu_resource.h"


namespace Renderer
{
	inline constexpr int SWAP_CHAIN_BUFFER_COUNT = 3;
	inline Resource::ColorBuffer g_DisplayPlane[SWAP_CHAIN_BUFFER_COUNT];
	inline ID3D12Device4* g_Device = nullptr;
	inline class DescHeap* g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];


	class DescHeap
	{
	public:
		DescHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);
		~DescHeap();
		std::tuple<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> Allocate(int count = 1);
	private:
		ID3D12DescriptorHeap* mDescHeap;
		UINT mDescSize;
		D3D12_CPU_DESCRIPTOR_HANDLE mCpuStart;
		D3D12_GPU_DESCRIPTOR_HANDLE mGpuStart;
		UINT mCurrentIndex;
	};

	class CommandAllocatorPool
	{
	public:
		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
		~CommandAllocatorPool();

		void Shutdown();

		ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
		void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

		inline size_t Size() { return m_AllocatorPool.size(); }

	private:
		const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

		std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
		std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
		std::mutex m_AllocatorMutex;
	};


	class CmdManager
	{
	public:
		CmdManager(ID3D12Device* InDevice);
		~CmdManager();
		ID3D12GraphicsCommandList* AllocateCmdList(D3D12_COMMAND_LIST_TYPE InType);
		ID3D12CommandAllocator* RequestAllocator(D3D12_COMMAND_LIST_TYPE InType, uint64_t CompletedFenceValue);
		ID3D12CommandQueue* GetQueue(D3D12_COMMAND_LIST_TYPE InType);
		void Discard(D3D12_COMMAND_LIST_TYPE InType,ID3D12CommandAllocator* cmdAllocator,uint64_t InFenceValue);
	private:
		ID3D12Device* mDevice = nullptr;
		std::vector<ID3D12GraphicsCommandList*> mCmdLists;
		std::map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandAllocatorPool>> mAllocatorMap;
		std::map<D3D12_COMMAND_LIST_TYPE, ID3D12CommandQueue*> mQueueMap;
	};

	class DeviceManager
	{
	public:
		DeviceManager();
		~DeviceManager();
		std::shared_ptr<CmdManager> GetCmdManager() const {
			return mCmdManager;
		};
		void SetTargetWindowAndCreateSwapChain(HWND InWindow,int InWidth,int InHeight);
		IDXGISwapChain1* GetSwapChain() { return s_SwapChain1; }
	private:
		void CreateSwapChain();
		void CreateCmdManager();
		void CreateD3DDevice();
	private:
		ID3D12Device4* mDevice = nullptr;
		IDXGISwapChain1* s_SwapChain1 = nullptr;
		std::shared_ptr<CmdManager> mCmdManager;
		int mWidth = 0;
		int mHeight = 0;
		HWND mWindow = nullptr;
	};

	
}