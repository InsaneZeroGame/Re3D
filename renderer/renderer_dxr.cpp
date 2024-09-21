#include "renderer_dxr.h"
#include "gui.h"
#include "BufferHelpers.h"


constexpr uint64_t GB = 1 << 30;
constexpr uint64_t MB = 1 << 20;
enum
{
	VERTEX_BUFFER_SIZE = 1 * GB,//1GB
	MESHLET_SIZE = 100 * MB,//100MB
	VERTEX_SIZE = 500 * MB,//500MB
	INDEX_SIZE = 200 * MB,//200MB
	PRIMITIVE_SIZE = 200 * MB,//200MB
};

struct Vertex {
	std::array<float, 3> position;
};

Renderer::DXRRenderer::DXRRenderer()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	Ensures(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)) == S_OK);
	Ensures(options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
	mGraphicsCmd = static_cast<ID3D12GraphicsCommandList4*>(mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT));
	mMeshShaderPass = std::make_shared<MeshShaderPass>(L"AmplifyShader.cso", L"MeshShader.cso", L"SimplePS.cso", nullptr);
	CreateBuffers();
}

Renderer::DXRRenderer::~DXRRenderer()
{

}

void Renderer::DXRRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	BaseRenderer::SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	if (GetContext())
	{
		GetContext()->CreateWindowDependentResource(InWidth, InHeight);
	}
	mGui = std::make_shared<Gui>(weak_from_this());
	mGui->CreateGui(mWindow);
	mDepthBuffer = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mDepthBuffer->Create(L"DepthBuffer", InWidth, InHeight, DXGI_FORMAT_D32_FLOAT);
	mCmdManager->AllocateCmdAndFlush(D3D12_COMMAND_LIST_TYPE_DIRECT, 
		[this](ID3D12GraphicsCommandList* InCmd, ID3D12CommandAllocator* InAllocator)
		{
			TransitState(InCmd, mDepthBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		});
}

void Renderer::DXRRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
	//BaseRenderer::LoadGameScene(InGameScene);
	mCurrentScene = InGameScene;
	entt::registry& sceneRegistery = mCurrentScene->GetRegistery();
	mGui->SetCurrentScene(InGameScene);
}

void Renderer::DXRRenderer::Update(float delta)
{
	
	UpdataFrameData();
	mDeviceManager->BeginFrame();
	auto lCurrentFrameIndex = mDeviceManager->GetCurrentFrameIndex();
	auto frameDataIndex = lCurrentFrameIndex % SWAP_CHAIN_BUFFER_COUNT;
	ID3D12CommandAllocator* lCmdAllocator =  mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mGraphicsFenceValue);
	mGraphicsCmd->Reset(lCmdAllocator, nullptr);
	TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentFrameIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentFrameIndex].GetRTV(), true, &mDepthBuffer->GetDSV());
	const float clearColor[] = { 0.6f, 0.8f, 0.4f, 1.0f };
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[lCurrentFrameIndex].GetRTV(),clearColor, 0, nullptr);
	mGraphicsCmd->ClearDepthStencilView(mDepthBuffer->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 1, & mRect);
	std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
	mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
	mGraphicsCmd->RSSetViewports(1, &mViewPort);
	mGraphicsCmd->RSSetScissorRects(1, &mRect);
	mMeshShaderPass->RenderScene(mGraphicsCmd);

	mGraphicsCmd->SetGraphicsRootDescriptorTable(0, mMeshletsBuffer.mSRVGpu);

	//Update Frame Resource
	TransitState(mGraphicsCmd, mFrameDataGPU[frameDataIndex]->GetResource(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mFrameDataGPU[frameDataIndex]->GetResource(), mFrameDataCPU[frameDataIndex]->GetResource());
	TransitState(mGraphicsCmd, mFrameDataGPU[frameDataIndex]->GetResource(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	mGraphicsCmd->SetGraphicsRootConstantBufferView(MESH_FRAME_DATA_ROOT_PARAMETER_INDEX, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());

	entt::registry& sceneRegistery = mCurrentScene->GetRegistery();
	auto allStaticMeshComponents = sceneRegistery.view<ECS::StaticMeshComponent, ECS::TransformComponent>();
	constexpr int matrixSizeNum32Bits = sizeof(DirectX::SimpleMath::Matrix) / 4;
	allStaticMeshComponents.each([this](auto entity, ECS::StaticMeshComponent& renderComponent, ECS::TransformComponent& transformComponent) {
		//Dispatch meshlets
		MeshShaderConstants meshConstants;
		meshConstants.mModelMatrix = transformComponent.GetModelMatrix();
		meshConstants.mMeshOffsets = renderComponent.mMeshOffsetWithinScene;
		mGraphicsCmd->SetGraphicsRoot32BitConstants(MESH_CONSTANTS_ROOT_PARAMETER_INDEX, matrixSizeNum32Bits, &meshConstants, 0);
		constexpr uint32_t meshletsPerGroup = 1;
		constexpr uint32_t maxThreadGroups = 128;
		ID3D12GraphicsCommandList6* meshCmd = static_cast<ID3D12GraphicsCommandList6*>(mGraphicsCmd);
		int i = 0;
		for (auto& meshlet : renderComponent.mMeshlets)
		{
			meshConstants.mMeshOffsets.MeshletOffset = renderComponent.mMeshOffsetWithinScene.MeshletOffset + i * MAX_MESHLET_PER_THREAD_GROUP;
			constexpr int offsetDatToSetIn32Bits = MESH_CONSTANTS_32BITS_NUM - matrixSizeNum32Bits;
			mGraphicsCmd->SetGraphicsRoot32BitConstants(MESH_CONSTANTS_ROOT_PARAMETER_INDEX, offsetDatToSetIn32Bits, &meshConstants.mMeshOffsets.MeshletOffset, matrixSizeNum32Bits);
			//meshCmd->DispatchMesh(MAX_MESHLET_PER_THREAD_GROUP, 1, 1);
			meshCmd->DispatchMesh(1, 1, 1);
			i++;
		}
		});
	
	//Gui
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

void Renderer::DXRRenderer::MeshShaderNewStaticmeshComponent(ECS::StaticMeshComponent& InStaticMeshComponent)
{
	mLoadResourceFuture = std::async(std::launch::async, [this, &InStaticMeshComponent]()
		{
			InStaticMeshComponent.ConvertToMeshlets(64, 126);
			UpdateScene(InStaticMeshComponent);
		});
}

void Renderer::DXRRenderer::CreateBuffers()
{
	BaseRenderer::CreateBuffers();
	D3D12_HEAP_DESC ldesc = {};
	ldesc.SizeInBytes = VERTEX_BUFFER_SIZE;
	ldesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ldesc.Alignment = 0;
	ldesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	g_Device->CreateHeap(&ldesc, IID_PPV_ARGS(&mMasterHeap));
	auto offset = 0;
	auto lBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(MESHLET_SIZE);
	g_Device->CreatePlacedResource(
		mMasterHeap,
		0,
		&lBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMeshletsBuffer.mBuffer));
	offset += MESHLET_SIZE;
	lBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VERTEX_SIZE);
	g_Device->CreatePlacedResource(
		mMasterHeap,
		offset,
		&lBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMeshletsVerticesBuffer.mBuffer));
	offset += VERTEX_SIZE;
	lBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(INDEX_SIZE);
	g_Device->CreatePlacedResource(
		mMasterHeap,
		offset,
		&lBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMeshletsIndicesBuffer.mBuffer));
	offset += INDEX_SIZE;
	lBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PRIMITIVE_SIZE);
	g_Device->CreatePlacedResource(
		mMasterHeap,
		offset,
		&lBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMeshletsPrimitivesBuffer.mBuffer));

	// Create SRVs for the buffers
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// Meshlets SRV
	{
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::Meshlet);
		srvDesc.Buffer.NumElements = static_cast<UINT>(MESHLET_SIZE / srvDesc.Buffer.StructureByteStride);
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		g_Device->CreateShaderResourceView(mMeshletsBuffer.mBuffer, &srvDesc, cpuHandle);
		mMeshletsBuffer.mSRV = cpuHandle;
		mMeshletsBuffer.mSRVGpu = gpuHandle;
	}

	// Meshlets vertices SRV
	{
		srvDesc.Buffer.StructureByteStride = sizeof(Renderer::Vertex);
		srvDesc.Buffer.NumElements = static_cast<UINT>(VERTEX_SIZE / srvDesc.Buffer.StructureByteStride);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsVerticesBuffer.mBuffer, &srvDesc, cpuHandle);
		mMeshletsVerticesBuffer.mSRV = cpuHandle;
		mMeshletsVerticesBuffer.mSRVGpu = gpuHandle;
	}

	// Meshlets indices SRV
	{
		srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
		srvDesc.Buffer.NumElements = static_cast<UINT>(INDEX_SIZE / srvDesc.Buffer.StructureByteStride);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsIndicesBuffer.mBuffer, &srvDesc, cpuHandle);
		mMeshletsIndicesBuffer.mSRV = cpuHandle;
		mMeshletsIndicesBuffer.mSRVGpu = gpuHandle;
	}
	// Meshlets primitives SRV
	{
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::MeshletTriangle);
		srvDesc.Buffer.NumElements = static_cast<UINT>(PRIMITIVE_SIZE / srvDesc.Buffer.StructureByteStride);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsPrimitivesBuffer.mBuffer, &srvDesc, cpuHandle);
		mMeshletsPrimitivesBuffer.mSRV = cpuHandle;
		mMeshletsPrimitivesBuffer.mSRVGpu = gpuHandle;
	}
}

HRESULT Renderer::DXRRenderer::UpdateScene(ECS::StaticMeshComponent& InStaticMeshComponent)
{
	std::lock_guard<std::mutex> lock(mLoadResourceMutex);

	InStaticMeshComponent.mMeshOffsetWithinScene = mCurrentMeshOffsets;

	mCurrentMeshOffsets.MeshletOffset += InStaticMeshComponent.mMeshlets.size() * MAX_MESHLET_PER_THREAD_GROUP;
	mCurrentMeshOffsets.VertexOffset += InStaticMeshComponent.mVertices.size();
	mCurrentMeshOffsets.PrimitiveOffset += InStaticMeshComponent.mMeshletPrimditives.size();
	mCurrentMeshOffsets.IndexOffset += InStaticMeshComponent.mMeshletsIndices.size();

	// Create committed resources for meshlets, vertices, indices, and primitives
	DirectX::ResourceUploadBatch resourceUpload(g_Device);
	// Begin the upload process
	resourceUpload.Begin();
	resourceUpload.Transition(mMeshletsBuffer.mBuffer, D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_COPY_DEST);
	resourceUpload.Transition(mMeshletsVerticesBuffer.mBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	resourceUpload.Transition(mMeshletsIndicesBuffer.mBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	resourceUpload.Transition(mMeshletsPrimitivesBuffer.mBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	
	int dataSize = InStaticMeshComponent.mMeshlets.size() * MAX_MESHLET_PER_THREAD_GROUP * sizeof(DirectX::Meshlet);
	UpdateMeshShaderResource(mMeshletsBuffer.mBuffer, 
		InStaticMeshComponent.mMeshlets.data(),dataSize,
		mMeshletsBuffer.mBufferOffsetInByte);
	mMeshletsBuffer.mBufferOffsetInByte += dataSize;

	dataSize = InStaticMeshComponent.mVertices.size() * sizeof(Renderer::Vertex);
	UpdateMeshShaderResource(mMeshletsVerticesBuffer.mBuffer, 
		InStaticMeshComponent.mVertices.data(),dataSize,
		mMeshletsVerticesBuffer.mBufferOffsetInByte);
	mMeshletsVerticesBuffer.mBufferOffsetInByte += dataSize;

	dataSize = InStaticMeshComponent.mMeshletsIndices.size() * sizeof(uint32_t);
	UpdateMeshShaderResource(mMeshletsIndicesBuffer.mBuffer, 
		InStaticMeshComponent.mMeshletsIndices.data(),dataSize,
		mMeshletsIndicesBuffer.mBufferOffsetInByte);
	mMeshletsIndicesBuffer.mBufferOffsetInByte += dataSize;

	dataSize = InStaticMeshComponent.mMeshletPrimditives.size() * sizeof(DirectX::MeshletTriangle);
	UpdateMeshShaderResource(mMeshletsPrimitivesBuffer.mBuffer, 
		InStaticMeshComponent.mMeshletPrimditives.data(),dataSize,
		mMeshletsPrimitivesBuffer.mBufferOffsetInByte);
	mMeshletsPrimitivesBuffer.mBufferOffsetInByte += dataSize;

	resourceUpload.Transition(mMeshletsBuffer.mBuffer, D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_GENERIC_READ);
	resourceUpload.Transition(mMeshletsVerticesBuffer.mBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	resourceUpload.Transition(mMeshletsIndicesBuffer.mBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	resourceUpload.Transition(mMeshletsPrimitivesBuffer.mBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	// End the upload process and wait for it to complete
	auto uploadFinished = resourceUpload.End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	uploadFinished.wait();
	return S_OK;
}

HRESULT Renderer::DXRRenderer::UpdateMeshShaderResource(
	ID3D12Resource* destResource,
	const void* srcData,
	size_t sizeInBytes,
	size_t destOffset)
{
	if (!destResource || !srcData)
	{
		return E_INVALIDARG;
	}

	// Create an intermediate upload buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
	auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	HRESULT hr = g_Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer));

	if (FAILED(hr))
	{
		return hr;
	}

	// Copy data to the upload buffer
	void* mappedData;
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	hr = uploadBuffer->Map(0, &readRange, &mappedData);
	if (FAILED(hr))
	{
		return hr;
	}

	memcpy(mappedData, srcData, sizeInBytes);
	uploadBuffer->Unmap(0, nullptr);

	// Create a command list to perform the copy operation
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	mCmdManager->AllocateCmdAndFlush(D3D12_COMMAND_LIST_TYPE_DIRECT, [destOffset, destResource, sizeInBytes,uploadBuffer](
		ID3D12GraphicsCommandList* commandList,ID3D12CommandAllocator* allocator) 
		{
			// Copy data from the upload buffer to the destination resource
			commandList->CopyBufferRegion(destResource, destOffset, uploadBuffer.Get(), 0, sizeInBytes);
		});
	return S_OK;
}


