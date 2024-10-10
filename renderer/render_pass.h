#pragma once
#include "renderer_context.h"

namespace DirectX 
{
	class ResourceUploadBatch;
}

namespace ECS
{
	struct StaticMeshComponent;
}

namespace Renderer
{
	class BaseRenderPass
	{
	public:
		BaseRenderPass(std::string_view InVertexShader, std::string_view InPixelShader, std::shared_ptr<RendererContext> InGraphicsContext = nullptr);
		virtual ~BaseRenderPass();
		virtual void RenderScene(ID3D12GraphicsCommandList* InCmdList) = 0;
		virtual void SetRenderPassStates(ID3D12GraphicsCommandList* InCmdList);
		virtual void CreatePipelineState() = 0;
		virtual void CreateRS();
		ID3D12PipelineState* GetPipelineState() { return mPipelineState; };
		ID3D12RootSignature* GetRS() { return mRS; };
	protected:
		virtual void DrawObject(const ECS::StaticMeshComponent& InAsset);
	protected:
		struct ID3D12PipelineState* mPipelineState;
		struct ID3D12RootSignature* mRS;
		std::unique_ptr<DirectX::ResourceUploadBatch> mUploadBuffer;
		struct D3D12_SHADER_BYTECODE mVertexShader;
		struct D3D12_SHADER_BYTECODE mPixelShader;
		ID3D12GraphicsCommandList* mGraphicsCmd;
		std::shared_ptr<RendererContext> mContext;
	};
	
}