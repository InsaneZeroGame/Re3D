#include "renderer_context.h"
const int SHADOW_MAP_WIDTH = 1920;
const int SHADOW_MAP_HEIGHT = 1080;

Renderer::RendererContext::RendererContext(std::shared_ptr<class CmdManager> InCmdManager):
	mCopyFenceValue(1),
	mCmdManager(InCmdManager)
{
	//1.Vertex Buffer
	mVertexBuffer = std::make_shared<Resource::VertexBuffer>();
	mVertexBuffer->Create(L"VertexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	mIndexBuffer = std::make_shared<Resource::VertexBuffer>();
	mIndexBuffer->Create(L"IndexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	//2.Upload Buffer
	mVertexBufferCpu = std::make_shared<VertexBufferRenderer<Vertex>>();
	mIndexBufferCpu = std::make_shared<VertexBufferRenderer<int>>();
	mCopyFenceHandle = CreateEvent(nullptr, false, false, nullptr);
	mCopyCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_COPY);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence)) == S_OK);
}

Renderer::RendererContext::~RendererContext()
{
}

void Renderer::RendererContext::CreateWindowDependentResource(int InWindowWidth, int InWindowHeight)
{
	mWindowHeight = InWindowHeight;
	mWindowWidth = InWindowWidth;
	mDepthBuffer = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mDepthBuffer->SetMsaaMode(MSAA_8X);
	mDepthBuffer->Create(L"DepthBuffer", mWindowWidth, mWindowHeight, DXGI_FORMAT_D32_FLOAT);
	CreateShadowMap(mWindowWidth, mWindowHeight);
	CreateColorBuffer(mWindowWidth, mWindowHeight);
}

void Renderer::RendererContext::UpdateDataToVertexBuffer(std::span<Vertex> InData)
{
	UploadDataToResource<Vertex>(mVertexBuffer->GetResource(), InData, mVertexBufferCpu);
}

void Renderer::RendererContext::UpdateDataToIndexBuffer(std::span<int> InData)
{
	UploadDataToResource<int>(mIndexBuffer->GetResource(), InData, mIndexBufferCpu);
}

std::shared_ptr<Renderer::Resource::ColorBuffer> Renderer::RendererContext::GetColorBuffer()
{
	return mColorBuffer;
}

std::shared_ptr<Renderer::Resource::DepthBuffer> Renderer::RendererContext::GetDepthBuffer()
{
	return mDepthBuffer;
}

std::shared_ptr<Renderer::Resource::DepthBuffer> Renderer::RendererContext::GetShadowMap()
{
	return mShadowMap;
}

std::shared_ptr<Renderer::Resource::VertexBuffer> Renderer::RendererContext::GetVertexBuffer()
{
	return mVertexBuffer;
}

std::shared_ptr<Renderer::Resource::VertexBuffer> Renderer::RendererContext::GetIndexBuffer()
{
	return mIndexBuffer;
}

std::shared_ptr<Renderer::VertexBufferRenderer<Renderer::Vertex>> Renderer::RendererContext::GetVertexBufferCpu()
{
	return mVertexBufferCpu;
}

std::shared_ptr<Renderer::VertexBufferRenderer<int>> Renderer::RendererContext::GetIndexBufferCpu()
{
	return mIndexBufferCpu;
}

void Renderer::RendererContext::CreateShadowMap(int InShadowMapWidth, int InShadowMapHeight)
{
	mShadowMap = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mShadowMap->Create(L"ShadowMap", InShadowMapWidth, InShadowMapHeight, DXGI_FORMAT_D32_FLOAT);
}

void Renderer::RendererContext::CreateColorBuffer(int InShadowMapWidth, int InShadowMapHeight)
{
	mColorBuffer = std::make_shared<Resource::ColorBuffer>();
	mColorBuffer->SetMsaaMode(MSAA_8X);
	mColorBuffer->Create(L"ColorBufferMSAA", mWindowWidth, mWindowHeight, 1, DXGI_FORMAT_R10G10B10A2_UNORM);
}

void Renderer::RendererContext::UploadDataToResource(
	ID3D12Resource* InDestResource,
	const void* data,
	uint64_t size,
	uint64_t InDestOffset)
{
	if (mCopyQueueUploadResourceSize < size)
	{
		if (mCopyQueueUploadResource)
		{
			mCopyQueueUploadResource->Release();
			mCopyQueueUploadResource = nullptr;
		}
		DirectX::CreateUploadBuffer(g_Device, data, size, 1, &mCopyQueueUploadResource);
		mCopyQueueUploadResourceSize = size;
	}
	ID3D12CommandAllocator* copyCmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_COPY, mCopyFenceValue);
	mCopyCmd->Reset(copyCmdAllocator, nullptr);
	void* Memory;
	auto range = CD3DX12_RANGE(0, size);
	mCopyQueueUploadResource->Map(0, &range, &Memory);
	memcpy(Memory, data, size);
	mCopyQueueUploadResource->Unmap(0, &range);
	mCopyCmd->CopyBufferRegion(InDestResource, InDestOffset, mCopyQueueUploadResource, 0, size);

	ID3D12CommandList* cmds[] = { mCopyCmd };
	mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY)->ExecuteCommandLists(1, cmds);
	mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_COPY, copyCmdAllocator, mCopyFenceValue);
	mCopyFenceValue++;
}

template<typename T>
void Renderer::RendererContext::UploadDataToResource(ID3D12Resource* InDestResource, std::span<T> InData, std::shared_ptr<VertexBufferRenderer<T>> InCpuResource)
{
	void* data = InData.data();
	uint64_t size = InData.size_bytes();
	if (mCopyQueueUploadResourceSize < size)
	{
		if (mCopyQueueUploadResource)
		{
			mCopyQueueUploadResource->Release();
			mCopyQueueUploadResource = nullptr;
		}
		DirectX::CreateUploadBuffer(g_Device, data, size, 1, &mCopyQueueUploadResource);
		mCopyQueueUploadResourceSize = size;
	}
	ID3D12CommandAllocator* copyCmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_COPY, mCopyFenceValue);
	mCopyCmd->Reset(copyCmdAllocator, nullptr);
	void* Memory;
	auto range = CD3DX12_RANGE(0, size);
	mCopyQueueUploadResource->Map(0, &range, &Memory);
	memcpy(Memory, data, size);
	mCopyQueueUploadResource->Unmap(0, &range);
	mCopyCmd->CopyBufferRegion(InDestResource, InCpuResource->GetOffsetBytes(), mCopyQueueUploadResource, 0, size);
	mCopyCmd->Close();
	ID3D12CommandList* cmds[] = { mCopyCmd };
	mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY)->ExecuteCommandLists(1, cmds);
	mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY)->Signal(mCopyFence, mCopyFenceValue);
	mCopyFence->SetEventOnCompletion(mCopyFenceValue, mCopyFenceHandle);
	WaitForSingleObject(mCopyFenceHandle, INFINITE);
	mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_COPY, copyCmdAllocator, mCopyFenceValue);
	mCopyFenceValue++;
	InCpuResource->UpdataDataOffset(InData);
}
