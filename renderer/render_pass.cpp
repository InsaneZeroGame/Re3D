#include "render_pass.h"

Renderer::BaseRenderPass::BaseRenderPass(const wchar_t* InVertexShader, const wchar_t* InPixelShader):
	mPipelineState(nullptr),
	mRS(nullptr),
	mUploadBuffer()
{
	Ensures(g_Device);
	mUploadBuffer = std::make_unique<DirectX::ResourceUploadBatch>(g_Device);
	mVertexShader = &Utils::ReadShader(InVertexShader);
	if (InPixelShader)
	{
		mPixelShader = &Utils::ReadShader(InPixelShader);
	}
}

Renderer::BaseRenderPass::~BaseRenderPass()
{

}
