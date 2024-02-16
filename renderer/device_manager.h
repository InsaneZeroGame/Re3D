#pragma once
#include <queue>
#include <map>

namespace Renderer
{

	class CommandAllocatorPool
	{
	public:
		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
		~CommandAllocatorPool();

		void Create(ID3D12Device* pDevice);
		void Shutdown();

		ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
		void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

		inline size_t Size() { return m_AllocatorPool.size(); }

	private:
		const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

		ID3D12Device* m_Device;
		std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
		std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
		std::mutex m_AllocatorMutex;
	};


	class CmdManager
	{
	public:
		CmdManager(ID3D12Device* InDevice);
		~CmdManager();
		ID3D12CommandList* AllocateCmdList(D3D12_COMMAND_LIST_TYPE InType);
		ID3D12CommandQueue* GetQueue(D3D12_COMMAND_LIST_TYPE InType);
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
		void SetTargetWindow(HWND InWindow,int InWidth,int InHeight);
	private:
		void CreateSwapChain();
		void CreateCmdManager();
		void CreateD3DDevice();
	private:
		ID3D12Device* mDevice = nullptr;
		IDXGISwapChain1* s_SwapChain1 = nullptr;
		std::unique_ptr<CmdManager> mCmdManager = nullptr;
		int mWidth = 0;
		int mHeight = 0;
		HWND mWindow = nullptr;
	};

	
}