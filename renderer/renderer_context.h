#pragma once
#include "BufferHelpers.h"
#include "components.h"


namespace Renderer
{
	namespace Resource
	{
		class VertexBuffer;
		class UploadBuffer;
		class DepthBuffer;
		class StructuredBuffer;
		class ColorBuffer;
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

	enum class RenderTarget
	{
		COLOR_OUTPUT_MSAA,
		COLOR_OUTPUT_RESOLVED,
		BLOOM_RES,
		BLUR_HALF,
		BLUR_QUAT
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
		std::shared_ptr<VertexBufferRenderer<uint32_t>> GetIndexBufferCpu();
		void CreateWindowDependentResource(int InWindowWidth, int InWindowHeight);
		void UpdateDataToVertexBuffer(std::span<Vertex> InData);
		void UpdateDataToIndexBuffer(std::span<uint32_t> InData);
		//std::shared_ptr<Resource::ColorBuffer> GetColorBuffer();
		//std::shared_ptr<Resource::ColorBuffer> GetColorAttachment0();
		std::shared_ptr<Resource::ColorBuffer> GetRenderTarget(RenderTarget InTarget);
		std::shared_ptr<class CmdManager> GetCmdManager();
		void LoadStaticMeshToGpu(ECS::StaticMeshComponent& InComponent);
	private:
		template<typename T>
		void UploadDataToResource(ID3D12Resource* InDestResource, std::span<T> InData, std::shared_ptr<VertexBufferRenderer<T>> InCpuResource);
		void UploadDataToResource(ID3D12Resource* InDestResource, const void* data, uint64_t size, uint64_t InDestOffset);
	private:
		std::shared_ptr<Resource::VertexBuffer> mVertexBuffer;
		std::shared_ptr<Resource::VertexBuffer> mIndexBuffer;
		std::shared_ptr<VertexBufferRenderer<Vertex>> mVertexBufferCpu;
		std::shared_ptr<VertexBufferRenderer<uint32_t>> mIndexBufferCpu;
		std::shared_ptr<Resource::DepthBuffer> mDepthBuffer;
		std::shared_ptr<Resource::DepthBuffer> mShadowMap;
		std::shared_ptr<Resource::ColorBuffer> mColorBufferMSAA;
		void CreateShadowMap(int InShadowMapWidth,int InShadowMapHeight);
		void CreateColorBuffer(int InShadowMapWidth, int InShadowMapHeight);
		int mWindowHeight;
		int mWindowWidth;
		uint64_t mCopyFenceValue;
		ID3D12Fence* mCopyFence;
		HANDLE mCopyFenceHandle;
		ID3D12GraphicsCommandList* mCopyCmd;
		ID3D12Resource* mCopyQueueUploadResource = nullptr;
		uint64_t mCopyQueueUploadResourceSize = 0;
		std::shared_ptr<class CmdManager> mCmdManager;
		enum 
		{
			MSAA_8X = 8,
		};
		std::shared_ptr<Resource::ColorBuffer> mColorAttachmentResolved;
		std::shared_ptr<Resource::ColorBuffer> mBloomBlurRTHalf;
		std::shared_ptr<Resource::ColorBuffer> mBloomBlurRTQuater;
		std::shared_ptr<Resource::ColorBuffer> mBloomRes;

	};
}