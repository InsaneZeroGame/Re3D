#include "renderer.h"
#include "device_manager.h"
#include "gpu_resource.h"
#include "obj_model_loader.h"
#include "utility.h"
#include <camera.h>
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
	mWidth = InWidth;
	mHeight = InHeight;
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	mDefaultCamera = std::make_unique<Gameplay::PerspectCamera>(InWidth, InHeight, 0.1, 125.0);
	mDefaultCamera->LookAt({ 0.0,1.0,2.0 }, { 0.0f,1.0f,0.0f }, { 0.0f,1.0f,0.0f });
}

void Renderer::BaseRenderer::Update(float delta)
{
	UpdataFrameData();
	IDXGISwapChain4* swapChain = (IDXGISwapChain4*)mDeviceManager->GetSwapChain();
	mCurrentBackbufferIndex = swapChain->GetCurrentBackBufferIndex();
	auto cmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,mFenceValue);
	mGraphicsCmd->Reset(cmdAllocator, mPipelineState);
	if (mIsFirstFrame)
	{
		FirstFrame();
		mIsFirstFrame = false;
	}
	TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), true, nullptr);
	D3D12_VIEWPORT lViewPort = {0,0,mWidth,mHeight,0.0,1.0};
	mGraphicsCmd->RSSetViewports(1, &lViewPort);
	D3D12_RECT lRect = {0,0,mWidth,mHeight };
	mGraphicsCmd->RSSetScissorRects(1, &lRect);
	float ColorRGBA[4] = { 0.15f,0.25f,0.75f,1.0f };
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), ColorRGBA, 1, &lRect);
	mGraphicsCmd->SetPipelineState(mPipelineState);
	mGraphicsCmd->SetGraphicsRootSignature(m_rootSignature);
	mGraphicsCmd->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mGraphicsCmd->SetGraphicsRootConstantBufferView(0, mFrameDataGPU->GetGpuVirtualAddress());
	RenderObject(mCurrentModel);

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
	//std::vector<Vertex> triangle = 
	//{
	//	{ {0.0,1.0,0.0,1.0},{1.0,0.0,0.0,1.0},{0.0,0.0}},
	//	{{ 1.0,0.0,0.0,1.0},{0.0,1.0,0.0,1.0},{0.0,0.0}},
	//	{{-1.0,0.0,0.0,1.0},{0.0,0.0,1.0,1.0},{0.0,0.0}},
	//};
	//
	//std::vector<uint32_t> indices = {0,1,2};
	AssetLoader::ObjModelLoader* objLoader = new AssetLoader::ObjModelLoader;
	auto model = objLoader->LoadAssetFromFile("cornell_box.obj");
	Ensures(model.has_value());
	mCurrentModel = model.value();
	auto& vertices = mCurrentModel.mMeshes[0].mVertices;
	auto& indices = mCurrentModel.mMeshes[0].mIndices;

	//auto& vertices = triangle;
	//1.Vertex Buffer
	mVertexBuffer = std::make_shared<Resource::VertexBuffer>();
	mVertexBuffer->Create(L"VertexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	mIndexBuffer = std::make_shared<Resource::VertexBuffer>();
	mIndexBuffer->Create(L"IndexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	//2.Upload Buffer
	mUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	constexpr int uploadBufferSize = MAX_ELE_COUNT * VERTEX_SIZE_IN_BYTE;
	mUploadBuffer->Create(L"UploadBuffer", uploadBufferSize);
	void* uploadBufferPtr = mUploadBuffer->Map();
	memcpy(uploadBufferPtr, vertices.data(), vertices.size() * sizeof(Vertex));
	mUploadBuffer->Unmap();

	mIndexUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	mIndexUploadBuffer->Create(L"UploadBuffer", uploadBufferSize);
	void* indexUploadBufferPtr = mIndexUploadBuffer->Map();
	memcpy(indexUploadBufferPtr, indices.data(), indices.size() * sizeof(uint32_t));
	mIndexUploadBuffer->Unmap();

	mFrameDataGPU = std::make_shared<Resource::UploadBuffer>();
	mFrameDataGPU->Create(L"FrameData", sizeof(mFrameDataCPU));
	mFrameDataPtr = mFrameDataGPU->Map();
}

void Renderer::BaseRenderer::FirstFrame()
{
	TransitState(mGraphicsCmd, mUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mVertexBuffer->GetResource(), mUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	
	TransitState(mGraphicsCmd, mIndexUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mIndexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mIndexBuffer->GetResource(), mIndexUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mIndexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);


}

void Renderer::BaseRenderer::PreRender()
{

}

void Renderer::BaseRenderer::PostRender()
{

}

void Renderer::BaseRenderer::CreatePipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = m_rootSignature;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = ReadShader(L"ForwardVS.cso");
	lDesc.PS = ReadShader(L"ForwardPS.cso");
	lDesc.SampleMask = UINT_MAX;

	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
	lDesc.DepthStencilState.DepthEnable = FALSE;
	lDesc.DepthStencilState.StencilEnable = FALSE;
	lDesc.InputLayout.NumElements = static_cast<UINT>(elements.size());
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
	
	D3D12_ROOT_PARAMETER frameDataCBV = {};
	frameDataCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	frameDataCBV.Descriptor.RegisterSpace = 0;
	frameDataCBV.Descriptor.ShaderRegister = 0;
	frameDataCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	std::vector<D3D12_ROOT_PARAMETER> parameters = 
	{
		frameDataCBV
	};

	rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
}

void Renderer::BaseRenderer::RenderObject(const AssetLoader::ModelAsset& InAsset)
{
	//Render 
	mGraphicsCmd->IASetVertexBuffers(0, 1, &mVertexBuffer->VertexBufferView());
	mGraphicsCmd->IASetIndexBuffer(&mIndexBuffer->IndexBufferView());
	for (const auto& mesh : InAsset.mMeshes)
	{
		mGraphicsCmd->DrawIndexedInstanced(mesh.mIndices.size(), 1, 0, 0, 0);
	}
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

D3D12_SHADER_BYTECODE Renderer::BaseRenderer::ReadShader(_In_z_ const wchar_t* name)
{
	auto shaderByteCode = Utility::ReadData(name);
	ID3DBlob* vertexBlob;
	D3DCreateBlob(shaderByteCode.size(), &vertexBlob);
	void* vertexPtr = vertexBlob->GetBufferPointer();
	memcpy(vertexPtr, shaderByteCode.data(), shaderByteCode.size());
	return CD3DX12_SHADER_BYTECODE(vertexBlob);
};

void Renderer::BaseRenderer::UpdataFrameData()
{
	mFrameDataCPU.mPrj = mDefaultCamera->GetPrj();
	mFrameDataCPU.mView = mDefaultCamera->GetView();
	mFrameDataCPU.mPrjView = mDefaultCamera->GetPrjView();
	memcpy(mFrameDataPtr,&mFrameDataCPU,sizeof(mFrameDataCPU));
}
