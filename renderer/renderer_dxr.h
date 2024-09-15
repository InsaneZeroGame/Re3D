#pragma once
#include "base_renderer.h"
#include "mesh_shader_pass.h"

namespace Renderer
{
	struct MeshShaderBuffer
	{
		ID3D12Resource* mBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE mSRV;
		D3D12_GPU_DESCRIPTOR_HANDLE mSRVGpu;
		uint64_t mBufferOffsetInByte = 0;
	};

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
		void CreateBuffers() override;
		HRESULT UpdateMeshShaderResource(ID3D12Resource* destResource,const void* srcData,size_t sizeInBytes,size_t destOffset);
		HRESULT UpdateScene(ECS::StaticMeshComponent& InStaticMeshComponent);
	private:
		ID3D12GraphicsCommandList4* mGraphicsCmd;
		std::shared_ptr<MeshShaderPass> mMeshShaderPass;
		MeshShaderBuffer mMeshletsBuffer;
		MeshShaderBuffer mMeshletsVerticesBuffer;
		MeshShaderBuffer mMeshletsIndicesBuffer;
		MeshShaderBuffer mMeshletsPrimitivesBuffer;
		bool sceneReady = false;
		//The heap other meshlet buffer allocated from
		ID3D12Heap* mMasterHeap;
		ECS::StaticMeshComponentMeshOffset mCurrentMeshOffsets;
		std::shared_ptr<Resource::DepthBuffer> mDepthBuffer;
		



	};
}