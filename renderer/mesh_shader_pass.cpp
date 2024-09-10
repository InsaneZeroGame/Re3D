#include "mesh_shader_pass.h"



Renderer::MeshShaderPass::MeshShaderPass(
	const wchar_t* InVertexShader, 
	const wchar_t* InPixelShader, 
	std::shared_ptr<RendererContext> InGraphicsContext) :BaseRenderPass(InVertexShader, InPixelShader, InGraphicsContext)
{
	// Define the triangle vertices
	struct Vertex {
		std::array<float,3> position;
	};

	Vertex triangleVertices[] = {
		{ { 0.0f,  0.5f, 0.0f } },
		{ { 0.5f, -0.5f, 0.0f } },
		{ {-0.5f, -0.5f, 0.0f } }
	};

	// Create the vertex buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(triangleVertices);
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	g_Device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	);

	// Copy the triangle vertices to the vertex buffer
	void* pData;
	vertexBuffer->Map(0, nullptr, &pData);
	memcpy(pData, triangleVertices, sizeof(triangleVertices));
	vertexBuffer->Unmap(0, nullptr);

	// Create the vertex buffer view
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(triangleVertices);
	vertexBufferView.StrideInBytes = sizeof(Vertex);
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
	InCmdList->SetGraphicsRootShaderResourceView(0, vertexBuffer->GetGPUVirtualAddress());
	ID3D12GraphicsCommandList6*  meshCmd = static_cast<ID3D12GraphicsCommandList6*>(InCmdList);
	meshCmd->DispatchMesh(1, 1, 1);
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
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
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
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];

	// Create a descriptor table for the SRV
	//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	//
	//// Create a root parameter and bind the descriptor table
	//rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

	rootParameters[0].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);
	// Create the root signature
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRS));

}

