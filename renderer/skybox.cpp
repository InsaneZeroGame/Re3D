#include "skybox.h"
#include "obj_model_loader.h"


Renderer::SkyboxPass::SkyboxPass(std::shared_ptr<RendererContext> InGraphicsContext):
	BaseRenderPass("SkyboxVS.hlsl","SkyboxPS.hlsl",InGraphicsContext),
	mStaticMeshComponent(nullptr)
{
	using namespace DirectX::DX12;
	Ensures(g_Device);
	CreateMesh();
	CreateRS();
	CreatePipelineState();
	CreateTextures();
}

void Renderer::SkyboxPass::CreateMesh()
{
	GeometricPrimitive::VertexCollection vertices;
	GeometricPrimitive::IndexCollection indices;
	GeometricPrimitive::CreateBox(vertices, indices, { 1.0f,1.0f,1.0f }, false, false);
	AssetLoader::ObjModelLoader* objLoader = AssetLoader::gObjModelLoader;
	auto mesh = objLoader->LoadAssetFromFile("cube.obj");
	Ensures(!mesh.empty());
	mStaticMeshComponent = std::make_shared<ECS::StaticMeshComponent>(std::move(mesh[0]));
	mContext->LoadStaticMeshToGpu(*mStaticMeshComponent);
}

void Renderer::SkyboxPass::CreateTextures()
{
	auto textureUploader = std::make_unique<ResourceUploadBatch>(g_Device);


	mSkyboxTexture = std::make_shared<Resource::Texture>();
	auto skybox_bottom = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/bottom.jpg");
	auto skybox_top = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/top.jpg");
	auto skybox_front = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/front.jpg");
	auto skybox_back = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/back.jpg");
	auto skybox_left = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/left.jpg");
	auto skybox_right = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/right.jpg");

	Ensures(skybox_bottom.has_value());
	Ensures(skybox_top.has_value());
	Ensures(skybox_front.has_value());
	Ensures(skybox_back.has_value());
	Ensures(skybox_left.has_value());
	Ensures(skybox_right.has_value());
	std::vector<AssetLoader::TextureData*> textures =
	{
		skybox_right.value(),
		skybox_left.value(),
		skybox_top.value(),
		skybox_bottom.value(),
		skybox_front.value(),
		skybox_back.value()
	};

	auto RowPitchBytes = textures[0]->mWidth * textures[0]->mComponent;
	mSkyboxTexture->CreateCube(RowPitchBytes, textures[0]->mWidth, textures[0]->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
	textureUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
	int i = 0;
	std::array<D3D12_SUBRESOURCE_DATA, 6> subResourceData;
	for (AssetLoader::TextureData* newTexture : textures)
	{
		D3D12_SUBRESOURCE_DATA& sourceData = subResourceData[i];
		sourceData.pData = newTexture->mdata;
		sourceData.RowPitch = RowPitchBytes;
		sourceData.SlicePitch = RowPitchBytes * newTexture->mHeight;
		++i;
	}
	textureUploader->Upload(mSkyboxTexture->GetResource(), 0, subResourceData.data(), subResourceData.size());
	//mBatchUploader->Transition(mSkyboxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	textureUploader->End(mContext->GetCmdManager()->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
}

Renderer::SkyboxPass::~SkyboxPass()
{

}

void Renderer::SkyboxPass::RenderScene(ID3D12GraphicsCommandList* InCmdList)
{
	mGraphicsCmd->SetGraphicsRootDescriptorTable(1, mSkyboxTexture->GetSRVGpu());
	DrawObject(*mStaticMeshComponent);
}

void Renderer::SkyboxPass::CreatePipelineState()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = mRS;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	lDesc.RasterizerState.FrontCounterClockwise = TRUE;
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = mVertexShader;
	lDesc.PS = mPixelShader;
	lDesc.SampleMask = UINT_MAX;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
	lDesc.InputLayout.NumElements = static_cast<UINT>(elements.size());
	lDesc.InputLayout.pInputElementDescs = elements.data();
	lDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lDesc.NumRenderTargets = 1;
	lDesc.RTVFormats[0] = g_ColorBufferFormat;
	//lDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	lDesc.SampleDesc.Count = 8;
	lDesc.SampleDesc.Quality = 0;
	lDesc.RasterizerState.MultisampleEnable = true;
	lDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	lDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineState));
	mPipelineState->SetName(L"Skybox PipelineState");
}

void Renderer::SkyboxPass::CreateRS()
{
	CD3DX12_ROOT_SIGNATURE_DESC lDesc = {};

	D3D12_ROOT_PARAMETER frameDataCBV = {};
	frameDataCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	frameDataCBV.Descriptor.RegisterSpace = 0;
	frameDataCBV.Descriptor.ShaderRegister = 0;
	frameDataCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_PARAMETER skyboxCubeTexture = {};
	skyboxCubeTexture.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	skyboxCubeTexture.DescriptorTable.NumDescriptorRanges = 1;
	D3D12_DESCRIPTOR_RANGE cubeTextureRange = {};
	cubeTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	cubeTextureRange.BaseShaderRegister = 1;
	cubeTextureRange.NumDescriptors = 1;
	cubeTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	cubeTextureRange.RegisterSpace = 0;
	skyboxCubeTexture.DescriptorTable.pDescriptorRanges = &cubeTextureRange;
	skyboxCubeTexture.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	std::vector<D3D12_ROOT_PARAMETER> lParameters = { frameDataCBV,skyboxCubeTexture };

	lDesc.pParameters = lParameters.data();
	lDesc.NumParameters = lParameters.size();


	D3D12_STATIC_SAMPLER_DESC cubeSampler = {};
	cubeSampler.ShaderRegister = 0;
	cubeSampler.RegisterSpace = 0;
	cubeSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	cubeSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	cubeSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	cubeSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	cubeSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;


	lDesc.Init((UINT)lParameters.size(), lParameters.data(), 1, &cubeSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3D12SerializeRootSignature(&lDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRS));
	mRS->SetName(L"SkyBox Root Signature");
}

void Renderer::SkyboxPass::SetEntity(entt::entity InEntity)
{
}

std::shared_ptr < ECS::StaticMeshComponent> Renderer::SkyboxPass::GetStaticMeshComponent()
{
	return mStaticMeshComponent;
}

ID3D12Resource* Renderer::SkyboxPass::GetSkyBoxTexture()
{
	return mSkyboxTexture->GetResource();
}

