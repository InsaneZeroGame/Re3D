#include "mesh_shader_pass.h"



Renderer::MeshShaderPass::MeshShaderPass(
	const wchar_t* InVertexShader, 
	const wchar_t* InPixelShader, 
	std::shared_ptr<RendererContext> InGraphicsContext) :BaseRenderPass(InVertexShader, InPixelShader, InGraphicsContext)
{
	
	CreateRS();
	CreatePipelineState();
}

Renderer::MeshShaderPass::~MeshShaderPass()
{
}

void Renderer::MeshShaderPass::RenderScene(ID3D12GraphicsCommandList* InCmdList)
{
	SetRenderPassStates(InCmdList);
	//InCmdList->SetGraphicsRootSignature(rootSignature.Get());
	//InCmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	
}

void Renderer::MeshShaderPass::CreatePipelineState()
{
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mRS;
	psoDesc.MS = mVertexShader;
	psoDesc.PS = mPixelShader;
	psoDesc.AS = { nullptr, 0 };
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	//psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc = DefaultSampleDesc();

	auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	g_Device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&mPipelineState));
}

void Renderer::MeshShaderPass::CreateRS()
{
	// Define the root signature
	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
	rootParameters.resize(3);
	D3D12_DESCRIPTOR_RANGE1 ranges[4];

	for (size_t i = 0; i < 4; i++)
	{
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[i].NumDescriptors = 1;
		ranges[i].BaseShaderRegister = i;
		ranges[i].RegisterSpace = 0;
		ranges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[i].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
	}
	rootParameters[0].InitAsDescriptorTable(4, ranges);
	rootParameters[1].InitAsConstants(MESH_CONSTANTS_32BITS_NUM, 0);
	rootParameters[2].InitAsConstantBufferView(1);
	// Create the root signature
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(rootParameters.size(), rootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ID3DBlob* signature;
	ID3DBlob* error;
	D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRS));
}

