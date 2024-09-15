#pragma once
#include "base_renderer.h"
#include "mesh_shader_pass.h"

namespace Renderer
{
	class DXRRenderer final:public std::enable_shared_from_this<DXRRenderer>,public BaseRenderer
	{
	public:
		DXRRenderer();
		~DXRRenderer();

		void SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight) override;

		void LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene) override;

		void Update(float delta) override;

		void MeshShaderNewStaticmeshComponent(ECS::StaticMeshComponent& InStaticMeshComponent) override;

	private:
		ID3D12GraphicsCommandList4* mGraphicsCmd;
		std::shared_ptr<MeshShaderPass> mMeshShaderPass;
		enum 
		{
			VERTEX_BUFFER_SIZE = 100000000//1GB
		};

		// New member variables for resources and SRVs
		ID3D12Resource* mMeshletsBuffer;
		ID3D12Resource* mMeshletsVerticesBuffer;
		ID3D12Resource* mMeshletsIndicesBuffer;
		ID3D12Resource* mMeshletsPrimitivesBuffer;

		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsVerticesSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsIndicesSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsPrimitivesSRV;

		D3D12_GPU_DESCRIPTOR_HANDLE mMeshletsSRVGpu;
		D3D12_GPU_DESCRIPTOR_HANDLE mMeshletsVerticesSRVGpu;
		D3D12_GPU_DESCRIPTOR_HANDLE mMeshletsIndicesSRVGpu;
		D3D12_GPU_DESCRIPTOR_HANDLE mMeshletsPrimitivesSRVGpu;

		HRESULT CreateResources(ECS::StaticMeshComponent& InStaticMeshComponent);
		HRESULT CreateSRVs(ECS::StaticMeshComponent& InStaticMeshComponent);
		bool sceneReady = false;
	};
}