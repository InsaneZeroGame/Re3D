#include "render_pass.h"

namespace Renderer
{
	struct MeshShaderConstants
	{
		DirectX::SimpleMath::Matrix mModelMatrix;
		ECS::StaticMeshComponentMeshOffset mMeshOffsets;
	};
	constexpr int MESH_VERTEX_ROOT_PARAMETER_INDEX = 0;
	constexpr int MESH_CONSTANTS_ROOT_PARAMETER_INDEX = 1;
	constexpr int MESH_FRAME_DATA_ROOT_PARAMETER_INDEX = 2;
	constexpr int MESH_CONSTANTS_32BITS_NUM = sizeof(MeshShaderConstants) / 4;
	

	class MeshShaderPass : public BaseRenderPass
	{
	public:
		MeshShaderPass(std::string_view InAmplifyShader,std::string_view InMeshShader, std::string_view InPixelShader, std::shared_ptr<RendererContext> InGraphicsContext);
		~MeshShaderPass();

	public:
		// Inherited via BaseRenderPass
		void RenderScene(ID3D12GraphicsCommandList* InCmdList) override;

		void CreatePipelineState() override;

		void CreateRS() override;

	private:
		struct D3D12_SHADER_BYTECODE mAmplifyShader;
	};
}