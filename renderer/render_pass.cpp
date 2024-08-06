#include "render_pass.h"
#include "components.h"

Renderer::BaseRenderPass::BaseRenderPass(const wchar_t* InVertexShader, const wchar_t* InPixelShader, std::shared_ptr<RendererContext> InGraphicsContext):
	mPipelineState(nullptr),
	mRS(nullptr),
	mContext(InGraphicsContext)
{
	Ensures(g_Device);
	mVertexShader = Utils::ReadShader(InVertexShader);
	if (InPixelShader)
	{
		mPixelShader = Utils::ReadShader(InPixelShader);
	}
}

Renderer::BaseRenderPass::~BaseRenderPass()
{

}

void Renderer::BaseRenderPass::SetRenderPassStates(ID3D12GraphicsCommandList* InCmdList)
{
	mGraphicsCmd = InCmdList;
	mGraphicsCmd->SetPipelineState(mPipelineState);
	mGraphicsCmd->SetGraphicsRootSignature(mRS);
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
