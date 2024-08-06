#pragma once
#include "render_pass.h"
#include "components.h"

namespace DirectX
{
	namespace DX12 
	{
		class GeometricPrimitive;
	}
}

namespace Renderer
{
	class SkyboxPass final: public BaseRenderPass
	{
	public:
		SkyboxPass(std::shared_ptr<RendererContext> InGraphicsContext);

		void CreateMesh();

		void CreateTextures();

		~SkyboxPass();

		void RenderScene(ID3D12GraphicsCommandList* InCmdList) override;

		void CreatePipelineState() override;

		void CreateRS() override;

		void SetEntity(entt::entity InEntity);

		std::shared_ptr < ECS::StaticMeshComponent> GetStaticMeshComponent();

		ID3D12Resource* GetSkyBoxTexture();
	private:
		std::unique_ptr<DirectX::DX12::GeometricPrimitive> mBox;

		std::shared_ptr<ECS::StaticMeshComponent> mStaticMeshComponent;

		std::shared_ptr<Resource::Texture> mSkyboxTexture;

	};
}