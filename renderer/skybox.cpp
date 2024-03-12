#include "skybox.h"



Renderer::Skybox::Skybox():BaseRenderPass(L"SkyboxVS.cso",L"SkyboxPS.cso")
{
	using namespace DirectX::DX12;
	Ensures(g_Device);
	GeometricPrimitive::VertexCollection vertices;
	GeometricPrimitive::IndexCollection indices;
	GeometricPrimitive::CreateBox(vertices, indices, { 1.0f,1.0f,1.0f }, false, false);
}

Renderer::Skybox::~Skybox()
{

}

void Renderer::Skybox::RenderScene()
{
}

void Renderer::Skybox::CreatePipelineState()
{
}

void Renderer::Skybox::CreateRS()
{
}

