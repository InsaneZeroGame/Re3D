#include "renderer.h"
#include "obj_model_loader.h"
#include "utility.h"
#include <camera.h>
#include "components.h"

Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mIsFirstFrame(true),
	mCurrentBackbufferIndex(0),
	mFenceValue(0),
	mFrameFence(nullptr),
	mComputeFence(nullptr),
	mGraphicsCmd(nullptr),
	mSkybox(nullptr),
	mComputeFenceValue(0),
	mCurrentScene(nullptr)
{
	Ensures(AssetLoader::gStbTextureLoader);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence)) == S_OK);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)) == S_OK);
	mFrameDoneEvent = CreateEvent(nullptr, false, false, nullptr);
	mComputeFenceHandle = CreateEvent(nullptr, false, false, nullptr);
	mGraphicsCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mBatchUploader = std::make_unique<ResourceUploadBatch>(g_Device);
	CreateBuffers();
	CreateTextures();
	CreateRootSignature();
	CreatePipelineState();
	CreateRenderTask();
	mSkybox = std::make_unique<Skybox>();
}

Renderer::BaseRenderer::~BaseRenderer()
{
	
}


void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	mWidth = InWidth;
	mHeight = InHeight;
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	//Use Reverse Z
	mDefaultCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f);
	//auto camera = std::make_shared<Gameplay::PerspectCamera>(InWidth, InHeight, 0.1,125);

	mDefaultCamera->LookAt({ 0.0,3.0,2.0 }, { 0.0f,1.0f,0.0f }, { 0.0f,1.0f,0.0f });

	mDepthBuffer = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mDepthBuffer->Create(L"DepthBuffer", mWidth, mHeight, DXGI_FORMAT_D32_FLOAT);
	mViewPort = { 0,0,(float)mWidth,(float)mHeight,0.0,1.0 };
	mRect = { 0,0,mWidth,mHeight };
}

void Renderer::BaseRenderer::Update(float delta)
{
	UpdataFrameData();
	mRenderExecution->run(*mRenderFlow).wait();
}
       
void Renderer::BaseRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
	mCurrentScene = InGameScene;
	entt::registry& sceneRegistery = mCurrentScene->GetRegistery();
	auto allStaticMeshComponents = sceneRegistery.view<ECS::StaticMeshComponent>();
	allStaticMeshComponents.each([this](auto entity, ECS::StaticMeshComponent& renderComponent) 
		{
			LoadStaticMeshToGpu(renderComponent);
		});
}

void Renderer::BaseRenderer::CreateRenderTask()
{
	mRenderFlow = std::make_unique<tf::Taskflow>();
	mRenderExecution = std::make_unique<tf::Executor>();

	auto DepthOnlyPass = [this]()
		{
			IDXGISwapChain4* swapChain = (IDXGISwapChain4*)mDeviceManager->GetSwapChain();
			//1.Reset CmdList
			mCurrentBackbufferIndex = swapChain->GetCurrentBackBufferIndex();
			mGraphicsCmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mFenceValue);
			mGraphicsCmd->Reset(mGraphicsCmdAllocator, mPipelineStateDepthOnly);
			if (mIsFirstFrame)
			{
				FirstFrame();
				mIsFirstFrame = false;
			}

			//Setup RenderTarget
			TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mGraphicsCmd->OMSetRenderTargets(0, nullptr, true, &mDepthBuffer->GetDSV());
			mGraphicsCmd->RSSetViewports(1, &mViewPort);
			mGraphicsCmd->RSSetScissorRects(1, &mRect);
			mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), mColorRGBA, 1, &mRect);
			mGraphicsCmd->ClearDepthStencilView(mDepthBuffer->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 1, &mRect);

			//Set Resources
			mGraphicsCmd->SetGraphicsRootSignature(mColorPassRootSignature);
			mGraphicsCmd->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			mGraphicsCmd->SetGraphicsRootConstantBufferView(0, mFrameDataGPU->GetGpuVirtualAddress());
			auto vbview = mVertexBuffer->VertexBufferView();
			mGraphicsCmd->IASetVertexBuffers(0, 1, &vbview);
			auto ibview = mIndexBuffer->IndexBufferView();
			mGraphicsCmd->IASetIndexBuffer(&ibview);
			mGraphicsCmd->SetPipelineState(mPipelineStateDepthOnly);
			using namespace ECS;
			auto& sceneRegistry = mCurrentScene->GetRegistery();
			auto renderEntities = sceneRegistry.view<StaticMeshComponent>();
			renderEntities.each([=](auto entity,auto component) 
				{
					RenderObject(component);
				});
		};

	auto ComputePass = [this]() 
		{
			ID3D12CommandAllocator* cmdAllcator = mDeviceManager->GetCmdManager()->RequestAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, mComputeFenceValue);
			mComputeCmd->Reset(cmdAllcator, mLightCullPass);
			mComputeCmd->SetPipelineState(mLightCullPass);
			mComputeCmd->SetComputeRootSignature(mLightCullPassRootSignature);
			mComputeCmd->SetComputeRootShaderResourceView(1, mLightBuffer->GetGpuVirtualAddress());
			mComputeCmd->SetComputeRootUnorderedAccessView(2, mClusterBuffer->GetGpuVirtualAddress());
			mComputeCmd->SetComputeRootConstantBufferView(0, mLightCullViewDataGpu->GetGpuVirtualAddress());
			mComputeCmd->Dispatch(CLUSTER_X, CLUSTER_Y, CLUSTER_Z);
			ID3D12CommandQueue* queue = mDeviceManager->GetCmdManager()->GetQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
			ID3D12CommandList* lCmds = { mComputeCmd };
			mComputeCmd->Close();
			queue->ExecuteCommandLists(1, &lCmds);
			queue->Signal(mComputeFence, mComputeFenceValue);
			mDeviceManager->GetCmdManager()->Discard(D3D12_COMMAND_LIST_TYPE_COMPUTE, cmdAllcator,mComputeFenceValue);
			mComputeFence->SetEventOnCompletion(mComputeFenceValue, mComputeFenceHandle);
			WaitForSingleObject(mComputeFenceHandle, INFINITE);
			mComputeFenceValue++;
		};

	auto ColorPass = [this]()
		{
			//Render Scene
			mGraphicsCmd->SetPipelineState(mColorPassPipelineState);
			mGraphicsCmd->SetGraphicsRootUnorderedAccessView(1, mClusterBuffer->GetGpuVirtualAddress());
			mGraphicsCmd->SetGraphicsRootShaderResourceView(2, mLightBuffer->GetGpuVirtualAddress());
			mGraphicsCmd->SetGraphicsRootConstantBufferView(3, mLightCullViewDataGpu->GetGpuVirtualAddress());
			std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
			mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
			mGraphicsCmd->SetGraphicsRootDescriptorTable(4, mDefaultTexture->GetSRVGpu());
			mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), true, &mDepthBuffer->GetDSV_ReadOnly());
			using namespace ECS;
			auto& sceneRegistry = mCurrentScene->GetRegistery();
			auto renderEntities = sceneRegistry.view<StaticMeshComponent>();
			renderEntities.each([=](auto entity, auto component)
				{
					RenderObject(component);
				});
		};

	auto PostRender = [this]()
		{
			//Flush CmdList
			TransitState(mGraphicsCmd, g_DisplayPlane[mCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			mGraphicsCmd->Close();
			ID3D12CommandList* cmds[] = { mGraphicsCmd };
			ID3D12CommandQueue* graphicsQueue = mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
			graphicsQueue->ExecuteCommandLists(1, cmds);
			graphicsQueue->Signal(mFrameFence, mFenceValue);

			//Wait For cmd finished to reclaim cmd
			mFrameFence->SetEventOnCompletion(mFenceValue, mFrameDoneEvent);
			WaitForSingleObject(mFrameDoneEvent, INFINITE);
			mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_DIRECT, mGraphicsCmdAllocator, mFenceValue);
			mFenceValue++;
			//Present
			IDXGISwapChain4* swapChain = (IDXGISwapChain4*)mDeviceManager->GetSwapChain();
			swapChain->Present(0, 0);
		};
	auto [depthOnlyPass, colorPass, postRender,computePass] = mRenderFlow->emplace(DepthOnlyPass,ColorPass,PostRender, ComputePass);
	colorPass.succeed(depthOnlyPass, computePass);
	postRender.succeed(colorPass);
}



void Renderer::BaseRenderer::CreateBuffers()
{
	//auto& vertices = triangle;
	//1.Vertex Buffer
	mVertexBuffer = std::make_shared<Resource::VertexBuffer>();
	mVertexBuffer->Create(L"VertexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	mIndexBuffer = std::make_shared<Resource::VertexBuffer>();
	mIndexBuffer->Create(L"IndexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	//2.Upload Buffer
	mVertexBufferCpu = std::make_shared<Resource::UploadBuffer>();
	constexpr int uploadBufferSize = MAX_ELE_COUNT * VERTEX_SIZE_IN_BYTE;
	mVertexBufferCpu->Create(L"UploadBuffer", uploadBufferSize);
	

	mIndexBufferCpu = std::make_shared<Resource::UploadBuffer>();
	mIndexBufferCpu->Create(L"UploadBuffer", uploadBufferSize);
	

	mFrameDataGPU = std::make_shared<Resource::UploadBuffer>();
	mFrameDataGPU->Create(L"FrameData", sizeof(mFrameDataCPU));

	mFrameDataCPU.DirectionalLightColor = SimpleMath::Vector4(1.0f, 1.0f, 1.0f,1.0f);
	mFrameDataCPU.DirectionalLightDir = SimpleMath::Vector4(1.0, 1.0, 2.0,1.0f);

	mLightBuffer = std::make_unique<Resource::StructuredBuffer>();
	mLightBuffer->Create(L"LightBuffer", (UINT32)mLights.size(), sizeof(ECS::LightComponent), nullptr);

	mLightUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	mLightUploadBuffer->Create(L"LightUploadBuffer", sizeof(ECS::LightComponent) * mLights.size());

	mLights[0].pos = { 0.0, 0.0, -5.0, 1.0f };
	mLights[0].radius_attenu = { 200.0, 0.0, 0.0, 1.0f };
	mLights[0].color = {1.0f,0.0f,0.0f,1.0f};

	mLights[1].pos = { -5.0, 0.0, 0.0, 1.0f };
	mLights[1].radius_attenu = { 200.0, 0.0, 0.0, 1.0f };
	mLights[1].color = { 0.0f,1.0f,0.0f,1.0f };

	mLights[3].pos = { 0.0, 0.0, 5.0, 1.0f };
	mLights[3].radius_attenu = { 50.0, 0.0, 0.0, 1.0f };
	mLights[3].color = { 1.0f,1.0f,0.0f,1.0f };

	mLights[2].pos = { 5.0, 0.0, 0.0, 1.0f };
	mLights[2].radius_attenu = { 10.0, 0.0, 0.0, 1.0f };
	mLights[2].color = { 1.0f,1.0f,1.0f,1.0f };
	float size = 30.0f;

	for (auto& light : mLights)
	{
		light.pos[0] = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;
		light.pos[1] = float(rand()) / RAND_MAX;
		light.pos[2] = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;

		light.pos[0] *= size;
		light.pos[1] *= 3.0f;
		light.pos[2] *= size;
		light.pos[3] = 1.0f;

		light.color[0] = float(rand()) / RAND_MAX;
		light.color[1] = float(rand()) / RAND_MAX;
		light.color[2] = float(rand()) / RAND_MAX;
		light.radius_attenu[0] = float(rand()) * 20.0f / RAND_MAX;
		light.radius_attenu[1] = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu[2] = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu[3] = float(rand()) * 1.2f / RAND_MAX;
	}

	//
	mLightUploadBuffer->UploadData<ECS::LightComponent>(mLights);

	mClusterBuffer = std::make_unique<Resource::StructuredBuffer>();
	mCLusters.resize(CLUSTER_X * CLUSTER_Y * CLUSTER_Z);
	mClusterBuffer->Create(L"ClusterBuffer", (UINT32)mCLusters.size(), sizeof(Cluster), nullptr);
	mLightCullViewDataGpu = std::make_unique<Resource::UploadBuffer>();
	mLightCullViewDataGpu->Create(L"LightCullViewData", sizeof(LightCullViewData));
}

void Renderer::BaseRenderer::CreateTextures()
{
	mDefaultTexture = std::make_shared<Resource::Texture>();
	auto defaultTexture = AssetLoader::gStbTextureLoader->LoadTextureFromFile("uvmap.png");
	if (defaultTexture.has_value())
	{
		AssetLoader::Texture* newTexture = defaultTexture.value();
		auto RowPitchBytes = newTexture->mWidth * newTexture->mComponent;
		mDefaultTexture->Create2D(RowPitchBytes, newTexture->mWidth, newTexture->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
		mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
		D3D12_SUBRESOURCE_DATA sourceData = {};
		sourceData.pData = newTexture->mdata;
		sourceData.RowPitch = RowPitchBytes;
		sourceData.SlicePitch = RowPitchBytes * newTexture->mHeight;
		mBatchUploader->Upload(mDefaultTexture->GetResource(), 0, &sourceData, 1);
		mBatchUploader->Transition(mDefaultTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
	}
}

void Renderer::BaseRenderer::DepthOnlyPass(const ECS::StaticMeshComponent& InAsset)
{

}

void Renderer::BaseRenderer::FirstFrame()
{
	TransitState(mGraphicsCmd, mVertexBufferCpu->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mVertexBuffer->GetResource(), mVertexBufferCpu->GetResource());
	TransitState(mGraphicsCmd, mVertexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	
	TransitState(mGraphicsCmd, mIndexBufferCpu->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mIndexBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mIndexBuffer->GetResource(), mIndexBufferCpu->GetResource());
	TransitState(mGraphicsCmd, mIndexBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	TransitState(mGraphicsCmd, mLightUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mLightBuffer->GetResource(), mLightUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	TransitState(mGraphicsCmd, mDepthBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	TransitState(mGraphicsCmd, mDefaultTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetFar(), mDefaultCamera->GetNear());
	mLightCullViewData.LightGridZParams = SimpleMath::Vector4(gridParas.x,gridParas.y, gridParas.z,0.0f);
	mLightCullViewData.ClipToView = mDefaultCamera->GetClipToView();
	mLightCullViewData.ViewMatrix = mDefaultCamera->GetView();
	mLightCullViewData.InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
	mLightCullViewDataGpu->UpdataData<LightCullViewData>(mLightCullViewData);
}

void Renderer::BaseRenderer::PreRender()
{

}

void Renderer::BaseRenderer::PostRender()
{

}

void Renderer::BaseRenderer::CreatePipelineState()
{
	//Color Pass pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = mColorPassRootSignature;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = Utils::ReadShader(L"ForwardVS.cso");
	lDesc.PS = Utils::ReadShader(L"ForwardPS.cso");
	lDesc.SampleMask = UINT_MAX;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
	lDesc.InputLayout.NumElements = static_cast<UINT>(elements.size());
	lDesc.InputLayout.pInputElementDescs = elements.data();
	lDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lDesc.NumRenderTargets = 1;
	lDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
	lDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	lDesc.SampleDesc.Count = 1;
	lDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	lDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mColorPassPipelineState));
	
	//Depth Only pipeline state
	lDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	lDesc.NumRenderTargets = 0;
	lDesc.PS = {};
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineStateDepthOnly));

	//Compute:Light Cull Pass
	D3D12_COMPUTE_PIPELINE_STATE_DESC lightCullPassDesc = {};
	lightCullPassDesc.CS = Utils::ReadShader(L"LightCull.cso");
	lightCullPassDesc.pRootSignature = mLightCullPassRootSignature;
	g_Device->CreateComputePipelineState(&lightCullPassDesc, IID_PPV_ARGS(&mLightCullPass));
}

void Renderer::BaseRenderer::CreateRootSignature()
{
	//Color Pass RS
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

		D3D12_ROOT_PARAMETER frameDataCBV = {};
		frameDataCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		frameDataCBV.Descriptor.RegisterSpace = 0;
		frameDataCBV.Descriptor.ShaderRegister = 0;
		frameDataCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_ROOT_PARAMETER clusterBuffer = {};
		clusterBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		clusterBuffer.Descriptor.RegisterSpace = 0;
		clusterBuffer.Descriptor.ShaderRegister = 2;
		clusterBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_PARAMETER lightBuffer = {};
		lightBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		lightBuffer.Descriptor.RegisterSpace = 0;
		lightBuffer.Descriptor.ShaderRegister = 1;
		lightBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_PARAMETER lightCullViewData = {};
		lightCullViewData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCullViewData.Descriptor.RegisterSpace = 0;
		lightCullViewData.Descriptor.ShaderRegister = 3;
		lightCullViewData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER diffuseColorTexture = {};
		diffuseColorTexture.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

		D3D12_DESCRIPTOR_RANGE diffuseRange = {};
		//Texture table range
		diffuseRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		diffuseRange.NumDescriptors = 1;
		diffuseRange.BaseShaderRegister = 4;
		diffuseRange.RegisterSpace = 0;
		//auto descSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//auto offsetInByte = mDefaultTexture->GetSRVGpu().ptr - g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap()->GetGPUDescriptorHandleForHeapStart().ptr;
		//auto offset = offsetInByte / descSize;
		diffuseRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges = { diffuseRange };
		diffuseColorTexture.DescriptorTable.NumDescriptorRanges = (UINT)ranges.size();
		diffuseColorTexture.DescriptorTable.pDescriptorRanges = ranges.data();
		diffuseColorTexture.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;



		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			frameDataCBV,//0
			clusterBuffer,//1
			lightBuffer,//2
			lightCullViewData,//3
			diffuseColorTexture//4
		};

		//Samplers
		D3D12_STATIC_SAMPLER_DESC texture2DSampler = CD3DX12_STATIC_SAMPLER_DESC(0);
		texture2DSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = { texture2DSampler };
		//rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), (UINT)samplers.size(), samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mColorPassRootSignature));
	}
	
	//Light Cull Pass RS
	{
		CD3DX12_ROOT_SIGNATURE_DESC lightCullRootSignatureDesc;
		D3D12_ROOT_PARAMETER lightBuffer = {};
		lightBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		lightBuffer.Descriptor.RegisterSpace = 0;
		lightBuffer.Descriptor.ShaderRegister = 1;
		lightBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER clusterBuffer = {};
		clusterBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		clusterBuffer.Descriptor.RegisterSpace = 0;
		clusterBuffer.Descriptor.ShaderRegister = 2;
		clusterBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER lightCullViewData = {};
		lightCullViewData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCullViewData.Descriptor.RegisterSpace = 0;
		lightCullViewData.Descriptor.ShaderRegister = 0;
		lightCullViewData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			lightCullViewData,lightBuffer,clusterBuffer
		};
		lightCullRootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		
		ID3DBlob* signature;
		ID3DBlob* error;
		D3D12SerializeRootSignature(&lightCullRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mLightCullPassRootSignature));
	}
	
}

void Renderer::BaseRenderer::RenderObject(const ECS::StaticMeshComponent& InAsset)
{
	//Render 
	mGraphicsCmd->DrawIndexedInstanced((UINT)InAsset.mIndexCount, 1, InAsset.StartIndexLocation, InAsset.BaseVertexLocation, 0);

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

void Renderer::BaseRenderer::UpdataFrameData()
{
	mFrameDataCPU.PrjView = mDefaultCamera->GetPrjView();
	mFrameDataCPU.View = mDefaultCamera->GetView();
	mFrameDataCPU.NormalMatrix = mDefaultCamera->GetNormalMatrix();
	mFrameDataCPU.DirectionalLightDir.Normalize();
	mFrameDataGPU->UpdataData<FrameData>(mFrameDataCPU);
	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetNear(), mDefaultCamera->GetFar());
	mLightCullViewData.LightGridZParams.x = gridParas.x;
	mLightCullViewData.LightGridZParams.y = gridParas.y;
	mLightCullViewData.LightGridZParams.z = gridParas.z;
	mLightCullViewData.ViewSizeAndInvSize = SimpleMath::Vector4((float)mWidth, (float)mHeight, 1.0f / (float)mWidth, 1.0f / (float)mHeight);
	mLightCullViewData.ClipToView = mDefaultCamera->GetClipToView();
	mLightCullViewData.ViewMatrix = mDefaultCamera->GetView();
	mLightCullViewData.InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
	mLightCullViewDataGpu->UpdataData<LightCullViewData>(mLightCullViewData);

}

void Renderer::BaseRenderer::LoadStaticMeshToGpu(ECS::StaticMeshComponent& InComponent)
{
	auto& vertices = InComponent.mVertices;
	auto& indices = InComponent.mIndices;
	InComponent.BaseVertexLocation = mVertexBufferCpu->GetOffset() / sizeof(Vertex);
	InComponent.StartIndexLocation = mIndexBufferCpu->GetOffset() / sizeof(int);
	mVertexBufferCpu->UploadData<Vertex>(vertices);
	mIndexBufferCpu->UploadData<int>(indices);
}
