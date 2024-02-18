#include "renderer.h"
#include "device_manager.h"
#include "gpu_resource.h"
#include <d3dcompiler.h>

constexpr int MAX_ELE_COUNT = 1000000;
constexpr int VERTEX_SIZE_IN_BYTE = sizeof(Renderer::Vertex);

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
	CreateBuffers();
	CreateRootSignature();
	CreatePipelineState();
}

Renderer::BaseRenderer::~BaseRenderer()
{

}

void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
}

void Renderer::BaseRenderer::Update(float delta)
{
	//spdlog::info("Update");
	IDXGISwapChain4* swapChain = (IDXGISwapChain4*)mDeviceManager->GetSwapChain();
	mCurrentBackbufferIndex = swapChain->GetCurrentBackBufferIndex();
	auto cmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,mFenceValue);
	mGraphicsCmd->Reset(cmdAllocator, nullptr);
	if (mIsFirstFrame)
	{
		FirstFrame();
		mIsFirstFrame = false;
	}
	TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), true, nullptr);
	float ColorRGBA[4] = { 0.15f,0.25f,0.75f,1.0f };
	D3D12_RECT rect = {0,0,800,600};
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), ColorRGBA, 1, &rect);
	
	//Render 
	{
		mGraphicsCmd->IASetVertexBuffers(0, 1, &mVertexBuffer->VertexBufferView());
		mGraphicsCmd->DrawInstanced(3, 1, 0, 0);
	}

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

void Renderer::BaseRenderer::CreateBuffers()
{
	//Triangle
	std::vector<Vertex> triangle = 
	{
		{{0.0,1.0,0.0,1.0},{1.0,0.0,0.0,1.0}},
		{{1.0,0.0,0.0,1.0},{0.0,1.0,0.0,1.0}},
		{{-1.0,0.0,0.0,1.0},{0.0,0.0,1.0,1.0}},

	};

	//1.Vertex Buffer
	mVertexBuffer = std::make_shared<Resource::VertexBuffer>();
	mVertexBuffer->Create(L"VertexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	//2.Upload Buffer
	mUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	constexpr int uploadBufferSize = MAX_ELE_COUNT * VERTEX_SIZE_IN_BYTE;
	mUploadBuffer->Create(L"UploadBuffer", uploadBufferSize);
	void* uploadBufferPtr = mUploadBuffer->Map();
	memcpy(uploadBufferPtr, triangle.data(), triangle.size());
	mUploadBuffer->Unmap();
}

void Renderer::BaseRenderer::FirstFrame()
{
	TransitState(mGraphicsCmd, mUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mVertexBuffer->GetResource(), mUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void Renderer::BaseRenderer::PreRender()
{

}

void Renderer::BaseRenderer::PostRender()
{

}
inline std::vector<uint8_t> ReadData(_In_z_ const wchar_t* name)
{
	std::ifstream inFile(name, std::ios::in | std::ios::binary | std::ios::ate);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	if (!inFile)
	{
		wchar_t moduleName[_MAX_PATH] = {};
		if (!GetModuleFileNameW(nullptr, moduleName, _MAX_PATH))
			throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "GetModuleFileNameW");

		wchar_t drive[_MAX_DRIVE];
		wchar_t path[_MAX_PATH];

		if (_wsplitpath_s(moduleName, drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0))
			throw std::runtime_error("_wsplitpath_s");

		wchar_t filename[_MAX_PATH];
		if (_wmakepath_s(filename, _MAX_PATH, drive, path, name, nullptr))
			throw std::runtime_error("_wmakepath_s");

		inFile.open(filename, std::ios::in | std::ios::binary | std::ios::ate);
	}
#endif

	if (!inFile)
		throw std::runtime_error("ReadData");

	const std::streampos len = inFile.tellg();
	if (!inFile)
		throw std::runtime_error("ReadData");

	std::vector<uint8_t> blob;
	blob.resize(size_t(len));

	inFile.seekg(0, std::ios::beg);
	if (!inFile)
		throw std::runtime_error("ReadData");

	inFile.read(reinterpret_cast<char*>(blob.data()), len);
	if (!inFile)
		throw std::runtime_error("ReadData");

	inFile.close();

	return blob;
}

D3D12_SHADER_BYTECODE ReadShader(_In_z_ const wchar_t* name)
{
	auto shaderByteCode = ReadData(name);
	ID3DBlob* vertexBlob;
	D3DCreateBlob(shaderByteCode.size(), &vertexBlob);
	void* vertexPtr = vertexBlob->GetBufferPointer();
	memcpy(vertexPtr, shaderByteCode.data(), shaderByteCode.size());
	return CD3DX12_SHADER_BYTECODE(vertexBlob);
};


void Renderer::BaseRenderer::CreatePipelineState()
{
	CHAR path[512];
	DWORD size = GetModuleFileName(nullptr, path, 512);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = m_rootSignature;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = ReadShader(L"ForwardVS.cso");
	lDesc.PS = ReadShader(L"ForwardPS.cso");

	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	lDesc.InputLayout.NumElements = elements.size();
	lDesc.InputLayout.pInputElementDescs = elements.data();
	lDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lDesc.NumRenderTargets = 1;
	lDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
	lDesc.SampleDesc.Count = 1;
	lDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineState));
}

void Renderer::BaseRenderer::CreateRootSignature()
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
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
