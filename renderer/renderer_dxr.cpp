#include "renderer_dxr.h"
#include "gui.h"

Renderer::DXRRenderer::DXRRenderer()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	Ensures(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)) == S_OK);
	Ensures(options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
	mGraphicsCmd = static_cast<ID3D12GraphicsCommandList4*>(mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT));
}

Renderer::DXRRenderer::~DXRRenderer()
{

}

void Renderer::DXRRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	BaseRenderer::SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	mContext->CreateWindowDependentResource(InWidth, InHeight);
	mGui = std::make_shared<Gui>(weak_from_this());
	mGui->CreateGui(mWindow);
}

void Renderer::DXRRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
}

void Renderer::DXRRenderer::Update(float delta)
{
	mDeviceManager->BeginFrame();
	auto lCurrentFrameIndex = mDeviceManager->GetCurrentFrameIndex();
	ID3D12CommandAllocator* lCmdAllocator =  mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mGraphicsFenceValue);
	mGraphicsCmd->Reset(lCmdAllocator, nullptr);
	TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentFrameIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentFrameIndex].GetRTV(), true, nullptr);
	const float clearColor[] = { 0.6f, 0.8f, 0.4f, 1.0f };
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[lCurrentFrameIndex].GetRTV(),clearColor, 0, nullptr);
	std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
	mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());

	mGui->BeginGui();
	mGui->Render();
	mGui->EndGui(mGraphicsCmd);

	
	TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentFrameIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mGraphicsCmd->Close();
	ID3D12CommandList* lCmdLists = { mGraphicsCmd };
	mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->ExecuteCommandLists(1,&lCmdLists);
	mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_DIRECT, lCmdAllocator, mGraphicsFenceValue);
	mGraphicsFenceValue++;
	mDeviceManager->EndFrame();
}
