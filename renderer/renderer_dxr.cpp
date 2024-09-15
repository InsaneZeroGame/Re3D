#include "renderer_dxr.h"
#include "gui.h"
#include "BufferHelpers.h"

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
	mMeshShaderPass = std::make_shared<MeshShaderPass>(L"MeshShader.cso", L"SimplePS.cso", nullptr);
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
	mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentFrameIndex].GetRTV(), true, nullptr);
	const float clearColor[] = { 0.6f, 0.8f, 0.4f, 1.0f };
	mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[lCurrentFrameIndex].GetRTV(),clearColor, 0, nullptr);
	std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
	mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
	mGraphicsCmd->RSSetViewports(1, &mViewPort);
	mGraphicsCmd->RSSetScissorRects(1, &mRect);
	mMeshShaderPass->RenderScene(mGraphicsCmd);

	if (sceneReady)
	{
		mGraphicsCmd->SetGraphicsRootDescriptorTable(0, mMeshletsSRVGpu);

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
			mGraphicsCmd->SetGraphicsRoot32BitConstants(MESH_CONSTANTS_ROOT_PARAMETER_INDEX, matrixSizeNum32Bits, &meshConstants, 0);
			constexpr uint32_t meshletsPerGroup = 1;
			constexpr uint32_t maxThreadGroups = 128;
			uint32_t totalMeshlets = renderComponent.mMeshlets.size();
			uint32_t meshletsProcessed = 0;
			ID3D12GraphicsCommandList6* meshCmd = static_cast<ID3D12GraphicsCommandList6*>(mGraphicsCmd);
			while (meshletsProcessed < totalMeshlets) {
				uint32_t meshletsToProcess = std::min(maxThreadGroups, totalMeshlets - meshletsProcessed);
				uint32_t dispatchX = (meshletsToProcess + meshletsPerGroup - 1) / 1; // meshletsPerGroup is 1
				// Update the meshlet offset for the current batch
				meshConstants.mMeshletOffset = renderComponent.mMeshletOffset + meshletsProcessed;
				mGraphicsCmd->SetGraphicsRoot32BitConstants(MESH_CONSTANTS_ROOT_PARAMETER_INDEX, 1, &meshConstants.mMeshletOffset, matrixSizeNum32Bits);
				meshCmd->DispatchMesh(dispatchX, 1, 1);
				meshletsProcessed += meshletsToProcess;
			}
			});
	}
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
	InStaticMeshComponent.ConvertToMeshlets(64, 126);
	CreateResources(InStaticMeshComponent);
	CreateSRVs(InStaticMeshComponent);
	sceneReady = true;
}

HRESULT Renderer::DXRRenderer::CreateResources(ECS::StaticMeshComponent& InStaticMeshComponent)
{
	// Create committed resources for meshlets, vertices, indices, and primitives
	HRESULT hr;
	DirectX::ResourceUploadBatch resourceUpload(g_Device);
	// Begin the upload process
	resourceUpload.Begin();

	// Meshlets buffer
	hr = DirectX::CreateStaticBuffer(
		g_Device,
		resourceUpload,
		InStaticMeshComponent.mMeshlets.data(),
		InStaticMeshComponent.mMeshlets.size() , sizeof(DirectX::Meshlet),
		D3D12_RESOURCE_STATE_COPY_DEST,
		&mMeshletsBuffer);
	if (FAILED(hr)) return hr;

	// Meshlets vertices buffer
	hr = DirectX::CreateStaticBuffer(
		g_Device,
		resourceUpload,
		InStaticMeshComponent.mMeshletsVertices.data(),
		InStaticMeshComponent.mMeshletsVertices.size() , sizeof(DirectX::XMFLOAT3),
		D3D12_RESOURCE_STATE_COPY_DEST,
		&mMeshletsVerticesBuffer);
	if (FAILED(hr)) return hr;

	// Meshlets indices buffer
	hr = DirectX::CreateStaticBuffer(
		g_Device,
		resourceUpload,
		InStaticMeshComponent.mMeshletsIndices.data(),
		InStaticMeshComponent.mMeshletsIndices.size() , sizeof(uint32_t),
		D3D12_RESOURCE_STATE_COPY_DEST,
		&mMeshletsIndicesBuffer);
	if (FAILED(hr)) return hr;

	// Meshlets primitives buffer
	hr = DirectX::CreateStaticBuffer(
		g_Device,
		resourceUpload,
		InStaticMeshComponent.mMeshletPrimditives.data(),
		InStaticMeshComponent.mMeshletPrimditives.size() , sizeof(DirectX::MeshletTriangle),
		D3D12_RESOURCE_STATE_COPY_DEST,
		&mMeshletsPrimitivesBuffer);
	if (FAILED(hr)) return hr;

	// End the upload process and wait for it to complete
	auto uploadFinished = resourceUpload.End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
	uploadFinished.wait();
	return S_OK;

}

HRESULT Renderer::DXRRenderer::CreateSRVs(ECS::StaticMeshComponent& InStaticMeshComponent)
{
	// Create SRVs for the buffers
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// Meshlets SRV
	{
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(InStaticMeshComponent.mMeshlets.size());
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::Meshlet);
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();

		g_Device->CreateShaderResourceView(mMeshletsBuffer, &srvDesc, cpuHandle);
		mMeshletsSRV = cpuHandle;
		mMeshletsSRVGpu = gpuHandle;
	}

	// Meshlets vertices SRV
	{
		srvDesc.Buffer.NumElements = static_cast<UINT>(InStaticMeshComponent.mMeshletsVertices.size());
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT3);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsVerticesBuffer, &srvDesc, cpuHandle);
		mMeshletsVerticesSRV = cpuHandle;
		mMeshletsVerticesSRVGpu = gpuHandle;
	}

	// Meshlets indices SRV
	{
		srvDesc.Buffer.NumElements = static_cast<UINT>(InStaticMeshComponent.mMeshletsIndices.size());
		srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsIndicesBuffer, &srvDesc, cpuHandle);
		mMeshletsIndicesSRV = cpuHandle;
		mMeshletsIndicesSRVGpu = gpuHandle;
	}
	// Meshlets primitives SRV
	{
		srvDesc.Buffer.NumElements = static_cast<UINT>(InStaticMeshComponent.mMeshletPrimditives.size());
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::MeshletTriangle);
		auto [cpuHandle, gpuHandle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
		g_Device->CreateShaderResourceView(mMeshletsPrimitivesBuffer, &srvDesc, cpuHandle);
		mMeshletsPrimitivesSRV = cpuHandle;
		mMeshletsPrimitivesSRVGpu = gpuHandle;
	}
	return S_OK;
}
