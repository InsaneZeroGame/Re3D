#include "renderer_context.h"
const int SHADOW_MAP_WIDTH = 1920;
const int SHADOW_MAP_HEIGHT = 1080;

Renderer::RendererContext::RendererContext()
{
	//1.Vertex Buffer
	mVertexBuffer = std::make_shared<Resource::VertexBuffer>();
	mVertexBuffer->Create(L"VertexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	mIndexBuffer = std::make_shared<Resource::VertexBuffer>();
	mIndexBuffer->Create(L"IndexBuffer", MAX_ELE_COUNT, VERTEX_SIZE_IN_BYTE);

	//2.Upload Buffer
	mVertexBufferCpu = std::make_shared<VertexBufferRenderer<Vertex>>();
	mIndexBufferCpu = std::make_shared<VertexBufferRenderer<int>>();
	CreateShadowMap(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
}

Renderer::RendererContext::~RendererContext()
{
}

void Renderer::RendererContext::CreateWindowDependentResource(int InWindowWidth, int InWindowHeight)
{
	mWindowHeight = InWindowHeight;
	mWindowWidth = InWindowWidth;
	mDepthBuffer = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mDepthBuffer->Create(L"DepthBuffer", mWindowWidth, mWindowHeight, DXGI_FORMAT_D32_FLOAT);
}

std::shared_ptr<Renderer::Resource::DepthBuffer> Renderer::RendererContext::GetDepthBuffer()
{
	return mDepthBuffer;
}

std::shared_ptr<Renderer::Resource::DepthBuffer> Renderer::RendererContext::GetShadowMap()
{
	return mShadowMap;
}

std::shared_ptr<Renderer::Resource::VertexBuffer> Renderer::RendererContext::GetVertexBuffer()
{
	return mVertexBuffer;
}

std::shared_ptr<Renderer::Resource::VertexBuffer> Renderer::RendererContext::GetIndexBuffer()
{
	return mIndexBuffer;
}

std::shared_ptr<Renderer::VertexBufferRenderer<Renderer::Vertex>> Renderer::RendererContext::GetVertexBufferCpu()
{
	return mVertexBufferCpu;
}

std::shared_ptr<Renderer::VertexBufferRenderer<int>> Renderer::RendererContext::GetIndexBufferCpu()
{
	return mIndexBufferCpu;
}

void Renderer::RendererContext::CreateShadowMap(int InShadowMapWidth, int InShadowMapHeight)
{
	mShadowMap = std::make_shared<Resource::DepthBuffer>(0.0f, 0);
	mShadowMap->Create(L"ShadowMap", InShadowMapWidth, InShadowMapHeight, DXGI_FORMAT_D32_FLOAT);
}
