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
	class Skybox final: public BaseRenderPass
	{
	public:
		Skybox();

		~Skybox();

		void RenderScene() override;

		void CreatePipelineState() override;

		void CreateRS() override;

		void SetEntity(entt::entity InEntity);

		std::shared_ptr < ECS::StaticMeshComponent> GetStaticMeshComponent();
	private:
		std::unique_ptr<DirectX::DX12::GeometricPrimitive> mBox;

		std::shared_ptr<ECS::StaticMeshComponent> mStaticMeshComponent;
	};
}