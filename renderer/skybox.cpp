#include "skybox.h"
#include "obj_model_loader.h"


Renderer::Skybox::Skybox():
	BaseRenderPass(L"SkyboxVS.cso",L"SkyboxPS.cso"),
	mStaticMeshComponent(nullptr)
{
	using namespace DirectX::DX12;
	Ensures(g_Device);
	GeometricPrimitive::VertexCollection vertices;
	GeometricPrimitive::IndexCollection indices;
	GeometricPrimitive::CreateBox(vertices, indices, { 1.0f,1.0f,1.0f }, false, false);
	AssetLoader::ObjModelLoader* objLoader = AssetLoader::gObjModelLoader;
	auto mesh = objLoader->LoadAssetFromFile("cube.obj");
	Ensures(mesh.has_value());
	mStaticMeshComponent = std::make_shared<ECS::StaticMeshComponent>(std::move(mesh.value()));
	CreateRS();
	CreatePipelineState();
}

Renderer::Skybox::~Skybox()
{

}

void Renderer::Skybox::RenderScene()
{
}

void Renderer::Skybox::CreatePipelineState()
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
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
	lDesc.InputLayout.NumElements = static_cast<UINT>(elements.size());
	lDesc.InputLayout.pInputElementDescs = elements.data();
	lDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lDesc.NumRenderTargets = 1;
	lDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
	//lDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	lDesc.SampleDesc.Count = 1;
	lDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	lDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineState));
	mPipelineState->SetName(L"Skybox PipelineState");
}

void Renderer::Skybox::CreateRS()
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

void Renderer::Skybox::SetEntity(entt::entity InEntity)
{
}

std::shared_ptr < ECS::StaticMeshComponent> Renderer::Skybox::GetStaticMeshComponent()
{
	return mStaticMeshComponent;
}


