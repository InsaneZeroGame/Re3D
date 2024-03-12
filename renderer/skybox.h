#pragma once
#include "render_pass.h"

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

	private:
		std::unique_ptr<DirectX::DX12::GeometricPrimitive> mBox;

	};
}