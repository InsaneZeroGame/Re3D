#pragma once

namespace DirectX 
{
	class ResourceUploadBatch;
}

namespace Renderer
{
	class BaseRenderPass
	{
	public:
		BaseRenderPass(const wchar_t* InVertexShader, const wchar_t* InPixelShader = nullptr);
		virtual ~BaseRenderPass();
		virtual void RenderScene() = 0;
		virtual void CreatePipelineState() = 0;
		virtual void CreateRS() = 0;
		ID3D12PipelineState* GetPipelineState() { return mPipelineState; };
		ID3D12RootSignature* GetRS() { return mRS; };
	protected:
		struct ID3D12PipelineState* mPipelineState;
		struct ID3D12RootSignature* mRS;
		std::unique_ptr<DirectX::ResourceUploadBatch> mUploadBuffer;
		struct D3D12_SHADER_BYTECODE mVertexShader;
		struct D3D12_SHADER_BYTECODE mPixelShader;
	};
	
}