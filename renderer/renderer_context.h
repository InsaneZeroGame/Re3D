#pragma once
#include "BufferHelpers.h"


namespace Renderer
{
	namespace Resource
	{
		class VertexBuffer;
		class UploadBuffer;
		class DepthBuffer;
		class StructuredBuffer;
	}

	template<typename T>
	struct VertexBufferRenderer
	{
		uint64_t GetOffset() { return mOffset; }
		uint64_t GetOffsetBytes() { return mOffsetBytes; }

		void UpdataDataOffset(std::span<T> InData)
		{
			mOffset += InData.size();
			mOffsetBytes += InData.size_bytes();
		};
	private:
		uint64_t mOffset = 0;
		uint64_t mOffsetBytes = 0;
	};

	class RendererContext
	{
	public:
		RendererContext(std::shared_ptr<class CmdManager> InCmdManager);
		~RendererContext();
		//Getters
		std::shared_ptr<Resource::DepthBuffer> GetDepthBuffer();
		std::shared_ptr<Resource::DepthBuffer> GetShadowMap();
		std::shared_ptr<Resource::VertexBuffer> GetVertexBuffer();
		std::shared_ptr<Resource::VertexBuffer> GetIndexBuffer();
		std::shared_ptr<VertexBufferRenderer<Vertex>> GetVertexBufferCpu();
		std::shared_ptr<VertexBufferRenderer<int>> GetIndexBufferCpu();
		void CreateWindowDependentResource(int InWindowWidth, int InWindowHeight);
		void UpdateDataToVertexBuffer(std::span<Vertex> InData);
		void UpdateDataToIndexBuffer(std::span<int> InData);

	private:
		template<typename T>
		void UploadDataToResource(ID3D12Resource* InDestResource, std::span<T> InData, std::shared_ptr<VertexBufferRenderer<T>> InCpuResource);
		void UploadDataToResource(ID3D12Resource* InDestResource, const void* data, uint64_t size, uint64_t InDestOffset);

	private:
		std::shared_ptr<Resource::VertexBuffer> mVertexBuffer;
		std::shared_ptr<Resource::VertexBuffer> mIndexBuffer;
		std::shared_ptr<VertexBufferRenderer<Vertex>> mVertexBufferCpu;
		std::shared_ptr<VertexBufferRenderer<int>> mIndexBufferCpu;
		std::shared_ptr<Resource::DepthBuffer> mDepthBuffer;
		std::shared_ptr<Resource::DepthBuffer> mShadowMap;
		void CreateShadowMap(int InShadowMapWidth,int InShadowMapHeight);
		int mWindowHeight;
		int mWindowWidth;
		uint64_t mCopyFenceValue;
		ID3D12Fence* mCopyFence;
		HANDLE mCopyFenceHandle;
		ID3D12GraphicsCommandList* mCopyCmd;
		ID3D12Resource* mCopyQueueUploadResource = nullptr;
		uint64_t mCopyQueueUploadResourceSize = 0;
		std::shared_ptr<class CmdManager> mCmdManager;
	};
}