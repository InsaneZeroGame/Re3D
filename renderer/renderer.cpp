#include "renderer.h"
#include "device_manager.h"
#include "gpu_resource.h"
#include "obj_model_loader.h"
#include "utility.h"
#include <camera.h>
#include <d3dcompiler.h>

constexpr int MAX_ELE_COUNT = 1000000;
constexpr int VERTEX_SIZE_IN_BYTE = sizeof(Renderer::Vertex);
constexpr int MAX_LIGHT_PER_TYPE = 32;
constexpr int CLUSTER_X = 32;
constexpr int CLUSTER_Y = 16;
constexpr int CLUSTER_Z = 16;


Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mIsFirstFrame(true),
	mCurrentBackbufferIndex(0),
	mFenceValue(0),
	mFrameFence(nullptr),
	mComputeFence(nullptr),
	mGraphicsCmd(nullptr),
	mComputeFenceValue(0)
{
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence)) == S_OK);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)) == S_OK);
	mFrameDoneEvent = CreateEvent(nullptr, false, false, nullptr);
	mComputeFenceHandle = CreateEvent(nullptr, false, false, nullptr);
	mGraphicsCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	CreateBuffers();
	CreateRootSignature();
	CreatePipelineState();
	CreateRenderTask();
}

Renderer::BaseRenderer::~BaseRenderer()
{
	
}

SimpleMath::Vector3 GetLightGridZParams(float NearPlane, float FarPlane)
{
	// S = distribution scale
	// B, O are solved for given the z distances of the first+last slice, and the # of slices.
	//
	// slice = log2(z*B + O) * S

	// Don't spend lots of resolution right in front of the near plane
	float NearOffset = .095f * 100;
	// Space out the slices so they aren't all clustered at the near plane
	float S = 4.05f;

	float N = NearPlane;
	float F = FarPlane;

	float O = (F - N * exp2((CLUSTER_Z - 1) / S)) / (F - N);
	float B = (1 - O) / N;

	return SimpleMath::Vector3(B, O, S);
}

SimpleMath::Vector4 CreateInvDeviceZToWorldZTransform(const SimpleMath::Matrix& ProjMatrix)
{
	// The perspective depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	//
	// Note a reversed Z projection matrix will have A=0.
	//
	// Done in shader as:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	float DepthMul = (float)ProjMatrix.m[2][2];
	float DepthAdd = (float)ProjMatrix.m[3][2];

	if (DepthAdd == 0.f)
	{
		// Avoid dividing by 0 in this case
		DepthAdd = 0.00000001f;
	}

	// perspective
	// SceneDepth = 1.0f / (DeviceZ / ProjMatrix.M[3][2] - ProjMatrix.M[2][2] / ProjMatrix.M[3][2])

	// ortho
	// SceneDepth = DeviceZ / ProjMatrix.M[2][2] - ProjMatrix.M[3][2] / ProjMatrix.M[2][2];

	// combined equation in shader to handle either
	// SceneDepth = DeviceZ * View.InvDeviceZToWorldZTransform[0] + View.InvDeviceZToWorldZTransform[1] + 1.0f / (DeviceZ * View.InvDeviceZToWorldZTransform[2] - View.InvDeviceZToWorldZTransform[3]);

	// therefore perspective needs
	// View.InvDeviceZToWorldZTransform[0] = 0.0f
	// View.InvDeviceZToWorldZTransform[1] = 0.0f
	// View.InvDeviceZToWorldZTransform[2] = 1.0f / ProjMatrix.M[3][2]
	// View.InvDeviceZToWorldZTransform[3] = ProjMatrix.M[2][2] / ProjMatrix.M[3][2]

	// and ortho needs
	// View.InvDeviceZToWorldZTransform[0] = 1.0f / ProjMatrix.M[2][2]
	// View.InvDeviceZToWorldZTransform[1] = -ProjMatrix.M[3][2] / ProjMatrix.M[2][2] + 1.0f
	// View.InvDeviceZToWorldZTransform[2] = 0.0f
	// View.InvDeviceZToWorldZTransform[3] = 1.0f

	bool bIsPerspectiveProjection = ProjMatrix.m[3][3] < 1.0f;

	if (bIsPerspectiveProjection)
	{
		float SubtractValue = DepthMul / DepthAdd;

		// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
		// This fixes fog not being applied to the black background in the editor.
		SubtractValue -= 0.00000001f;

		return SimpleMath::Vector4(
			0.0f,
			0.0f,
			1.0f / DepthAdd,
			SubtractValue
		);
	}
	else
	{
		return SimpleMath::Vector4(
			(float)(1.0f / ProjMatrix.m[2][2]),
			(float)(-ProjMatrix.m[3][2] / ProjMatrix.m[2][2] + 1.0f),
			0.0f,
			1.0f
		);
	}
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
			mGraphicsCmd->IASetVertexBuffers(0, 1, &mVertexBuffer->VertexBufferView());
			mGraphicsCmd->IASetIndexBuffer(&mIndexBuffer->IndexBufferView());
			mGraphicsCmd->SetPipelineState(mPipelineStateDepthOnly);
			RenderObject(mCurrentModel);
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
			WaitForSingleObject(mComputeFenceHandle,INFINITE);
			mComputeFenceValue++;
		};

	auto ColorPass = [this]()
		{
			//Render Scene
			mGraphicsCmd->SetPipelineState(mColorPassPipelineState);
			mGraphicsCmd->SetGraphicsRootUnorderedAccessView(1, mClusterBuffer->GetGpuVirtualAddress());
			mGraphicsCmd->SetGraphicsRootShaderResourceView(2, mLightBuffer->GetGpuVirtualAddress());
			mGraphicsCmd->SetGraphicsRootConstantBufferView(3, mLightCullViewDataGpu->GetGpuVirtualAddress());
			mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[mCurrentBackbufferIndex].GetRTV(), true, &mDepthBuffer->GetDSV_ReadOnly());
			RenderObject(mCurrentModel);
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
	depthOnlyPass.precede(colorPass);
	computePass.precede(colorPass);
	colorPass.precede(postRender);
}



void Renderer::BaseRenderer::CreateBuffers()
{
	AssetLoader::ObjModelLoader* objLoader = new AssetLoader::ObjModelLoader;
	auto model = objLoader->LoadAssetFromFile("scene.obj");
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

	mFrameDataCPU.DirectionalLightColor = SimpleMath::Vector4(1.0f, 1.0f, 1.0f,1.0f);
	mFrameDataCPU.DirectionalLightDir = SimpleMath::Vector4(1.0, 1.0, 2.0,1.0f);

	mLightBuffer = std::make_unique<Resource::StructuredBuffer>();
	mLightBuffer->Create(L"LightBuffer", (UINT32)mLights.size(), sizeof(Light), nullptr);

	mLightUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	mLightUploadBuffer->Create(L"LightUploadBuffer", sizeof(Light) * mLights.size());

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
	void* lightUploadBufferPtr = mLightUploadBuffer->Map();
	memcpy(lightUploadBufferPtr, mLights.data(), mLights.size() * sizeof(Light));
	mLightUploadBuffer->Unmap();

	mClusterBuffer = std::make_unique<Resource::StructuredBuffer>();
	mCLusters.resize(CLUSTER_X * CLUSTER_Y * CLUSTER_Z);
	mClusterBuffer->Create(L"ClusterBuffer", (UINT32)mCLusters.size(), sizeof(Cluster), nullptr);

	
	mLightCullViewDataGpu = std::make_unique<Resource::UploadBuffer>();
	mLightCullViewDataGpu->Create(L"LightCullViewData", sizeof(LightCullViewData));

	mLightCullDataPtr = mLightCullViewDataGpu->Map();

}

void Renderer::BaseRenderer::DepthOnlyPass(const AssetLoader::ModelAsset& InAsset)
{

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

	TransitState(mGraphicsCmd, mLightUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mLightBuffer->GetResource(), mLightUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	TransitState(mGraphicsCmd, mDepthBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	auto gridParas = GetLightGridZParams(mDefaultCamera->GetFar(), mDefaultCamera->GetNear());
	mLightCullViewData.LightGridZParams = 
		SimpleMath::Vector4(gridParas.x,
			gridParas.y, gridParas.z,0.0f);
	mLightCullViewData.ClipToView = mDefaultCamera->GetClipToView();
	mLightCullViewData.ViewMatrix = mDefaultCamera->GetView();
	mLightCullViewData.InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));

	memcpy(mLightCullDataPtr, &mLightCullViewData, sizeof(LightCullViewData));
	
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
	lDesc.VS = ReadShader(L"ForwardVS.cso");
	lDesc.PS = ReadShader(L"ForwardPS.cso");
	lDesc.SampleMask = UINT_MAX;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

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
	lightCullPassDesc.CS = ReadShader(L"LightCull.cso");
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

		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			frameDataCBV,clusterBuffer,lightBuffer,lightCullViewData
		};

		rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void Renderer::BaseRenderer::RenderObject(const AssetLoader::ModelAsset& InAsset)
{
	//Render 
	
	for (const auto& mesh : InAsset.mMeshes)
	{
		mGraphicsCmd->DrawIndexedInstanced((UINT)mesh.mIndices.size(), 1, 0, 0, 0);
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
	//mFrameDataCPU.mPrj = mDefaultCamera->GetPrj();
	//mFrameDataCPU.mView = mDefaultCamera->GetView();
	mFrameDataCPU.PrjView = mDefaultCamera->GetPrjView();
	mFrameDataCPU.View = mDefaultCamera->GetView();
	mFrameDataCPU.NormalMatrix = mDefaultCamera->GetNormalMatrix();
	mFrameDataCPU.DirectionalLightDir.Normalize();
	memcpy(mFrameDataPtr,&mFrameDataCPU,sizeof(mFrameDataCPU));
	auto gridParas = GetLightGridZParams(mDefaultCamera->GetNear(), mDefaultCamera->GetFar());
	mLightCullViewData.LightGridZParams.x = gridParas.x;
	mLightCullViewData.LightGridZParams.y = gridParas.y;
	mLightCullViewData.LightGridZParams.z = gridParas.z;
	mLightCullViewData.ViewSizeAndInvSize = SimpleMath::Vector4((float)mWidth, (float)mHeight, 1.0f / (float)mWidth, 1.0f / (float)mHeight);
	mLightCullViewData.ClipToView = mDefaultCamera->GetClipToView();
	mLightCullViewData.ViewMatrix = mDefaultCamera->GetView();
	mLightCullViewData.InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
	memcpy(mLightCullDataPtr, &mLightCullViewData, sizeof(LightCullViewData));
}
