#include "renderer.h"
#include "device_manager.h"


Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mIsFirstFrame(true),
	mCurrentBackbufferIndex(0),
	mFenceValue(0),
	mFrameFence(nullptr),
	mGraphicsCmd(nullptr)
{
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence)) == S_OK);
	mFrameDoneEvent = CreateEvent(nullptr, false, false, nullptr);
	auto newAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 0);
	mGraphicsCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT, newAllocator);
}

Renderer::BaseRenderer::~BaseRenderer()
{

}

void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	//std::array<ID3D12CommandList*, SWAP_CHAIN_BUFFER_COUNT> cmdArray;
	//std::array<ID3D12CommandAllocator*, SWAP_CHAIN_BUFFER_COUNT> cmdAllocatorArray;
	//
	//for (auto i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	//{
	//	auto [mGraphicsCmd, cmdAllocator] = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//	cmdArray[i] = mGraphicsCmd;
	//	cmdAllocatorArray[i] = cmdAllocator;
	//	mGraphicsCmd->Reset(cmdAllocator, nullptr);
	//	D3D12_RESOURCE_BARRIER barrier= {};
	//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//	barrier.Transition.Subresource = 0;
	//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//	barrier.Transition.pResource = g_DisplayPlane[i].GetResource();
	//	mGraphicsCmd->ResourceBarrier(1, &barrier);	
	//	mGraphicsCmd->Close();
	//
	//}
	//mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->ExecuteCommandLists(cmdArray.size(), cmdArray.data());
	//for (auto i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	//{
	//	mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocatorArray[i]);
	//}
}

void Renderer::BaseRenderer::Update(float delta)
{
	//spdlog::info("Update");
	IDXGISwapChain4* swapChain = (IDXGISwapChain4*)mDeviceManager->GetSwapChain();
	mCurrentBackbufferIndex = swapChain->GetCurrentBackBufferIndex();
	auto cmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,mFenceValue);
	mGraphicsCmd->Reset(cmdAllocator, nullptr);
	TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), true, nullptr);
	float ColorRGBA[4] = { 0.15f,0.25f,0.75f,1.0f };
	D3D12_RECT rect = {0.0,0.0f,800,600};
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), ColorRGBA, 1, &rect);
	TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mGraphicsCmd->Close();
	ID3D12CommandList* cmds[] = { mGraphicsCmd };
	ID3D12CommandQueue* graphicsQueue =  mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	graphicsQueue->ExecuteCommandLists(1, cmds);
	graphicsQueue->Signal(mFrameFence, mFenceValue);
	mFrameFence->SetEventOnCompletion(mFenceValue, mFrameDoneEvent);
	WaitForSingleObject(mFrameDoneEvent, INFINITE);
	mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_DIRECT,cmdAllocator, mFenceValue);
	mFenceValue++;
	swapChain->Present(0, 0);
}

void Renderer::BaseRenderer::FirstFrame(ID3D12GraphicsCommandList* InCmd)
{
	
}

void Renderer::BaseRenderer::PreRender()
{

}

void Renderer::BaseRenderer::PostRender()
{

}

void Renderer::BaseRenderer::TransitState(ID3D12GraphicsCommandList* InCmd, ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.Subresource = 0;
	barrier.Transition.StateBefore = InBefore;
	barrier.Transition.StateAfter = InAfter;
	barrier.Transition.pResource = InResource;
	InCmd->ResourceBarrier(1, &barrier);
}
