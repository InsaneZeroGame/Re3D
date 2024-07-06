#pragma once


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

		void UpdataData(std::span<T> InData)
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
		RendererContext();
		~RendererContext();
		void CreateWindowDependentResource(int InWindowWidth,int InWindowHeight);
		std::shared_ptr<Resource::DepthBuffer> GetDepthBuffer();
		std::shared_ptr<Resource::DepthBuffer> GetShadowMap();
		std::shared_ptr<Resource::VertexBuffer> GetVertexBuffer();
		std::shared_ptr<Resource::VertexBuffer> GetIndexBuffer();
		std::shared_ptr<VertexBufferRenderer<Vertex>> GetVertexBufferCpu();
		std::shared_ptr<VertexBufferRenderer<int>> GetIndexBufferCpu();
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

	};
}