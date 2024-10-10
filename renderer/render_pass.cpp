#include "render_pass.h"
#include "components.h"

Renderer::BaseRenderPass::BaseRenderPass(std::string_view InVertexShader, std::string_view InPixelShader, std::shared_ptr<RendererContext> InGraphicsContext):
	mPipelineState(nullptr),
	mRS(nullptr),
	mContext(InGraphicsContext)
{
	Ensures(g_Device);
	if (!InVertexShader.empty())
	{
		mVertexShader = Utils::ReadShader(InVertexShader, "main", "vs_6_5");
	}
	if (!InPixelShader.empty())
	{
		mPixelShader = Utils::ReadShader(InPixelShader, "main", "ps_6_5");
	}
}

Renderer::BaseRenderPass::~BaseRenderPass()
{

}

void Renderer::BaseRenderPass::SetRenderPassStates(ID3D12GraphicsCommandList* InCmdList)
{
	mGraphicsCmd = InCmdList;
	mGraphicsCmd->SetPipelineState(mPipelineState);
	if (mRS)
	{
		mGraphicsCmd->SetGraphicsRootSignature(mRS);
	}
}

void Renderer::BaseRenderPass::CreateRS()
{
	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRS));

}

void Renderer::BaseRenderPass::DrawObject(const ECS::StaticMeshComponent& InAsset)
{
	//Render 
	for (const auto& subMesh : InAsset.mSubMeshes)
	{
		mGraphicsCmd->DrawIndexedInstanced((UINT)subMesh.second.TriangleCount * 3, 1, InAsset.StartIndexLocation + subMesh.second.IndexOffset,
			InAsset.BaseVertexLocation, 0);
	}
}
