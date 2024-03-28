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
		virtual void RenderScene() {};
		virtual void CreatePipelineState() {};
		virtual void CreateRS() {};
	protected:
		struct ID3D12PipelineState* mPipelineState;
		struct ID3D12RootSignature* mRS;
		std::unique_ptr<DirectX::ResourceUploadBatch> mUploadBuffer;
		struct D3D12_SHADER_BYTECODE mVertexShader;
		struct D3D12_SHADER_BYTECODE mPixelShader;
	};
	
}