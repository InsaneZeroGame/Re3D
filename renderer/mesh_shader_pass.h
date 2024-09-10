#include "render_pass.h"

namespace Renderer
{
	class MeshShaderPass : public BaseRenderPass
	{
	public:
		MeshShaderPass(const wchar_t* InVertexShader, const wchar_t* InPixelShader, std::shared_ptr<RendererContext> InGraphicsContext);
		~MeshShaderPass();

	public:
		// Inherited via BaseRenderPass
		void RenderScene(ID3D12GraphicsCommandList* InCmdList) override;

		void CreatePipelineState() override;

		void CreateRS() override;

	private:
		ID3D12Resource* vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

	};
}