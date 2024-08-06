#pragma once
#include "render_pass.h"


namespace Renderer
{
	class LightCullPass final : public BaseRenderPass
	{
	public:
		LightCullPass(std::shared_ptr<RendererContext> InGraphicsContext);

		~LightCullPass();

		void RenderScene(ID3D12GraphicsCommandList* InCmdList) override;

		void CreatePipelineState() override;

		void CreateRS() override;

		void SetRenderPassStates(ID3D12GraphicsCommandList* InCmdList) override;

	private:

	};
}