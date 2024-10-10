#include "light_cull_pass.h"



Renderer::LightCullPass::LightCullPass(std::shared_ptr<RendererContext> InGraphicsContext):
	BaseRenderPass("", "", InGraphicsContext)
{
	mVertexShader = Utils::ReadShader("LightCull.hlsl", "main", "cs_6_5");
	CreateRS();
	CreatePipelineState();
}

Renderer::LightCullPass::~LightCullPass()
{

}

void Renderer::LightCullPass::RenderScene(ID3D12GraphicsCommandList* InCmdList)
{
	mGraphicsCmd->Dispatch(CLUSTER_X, CLUSTER_Y, CLUSTER_Z);

}

void Renderer::LightCullPass::CreatePipelineState()
{
	//Compute:Light Cull Pass
	D3D12_COMPUTE_PIPELINE_STATE_DESC lightCullPassDesc = {};
	lightCullPassDesc.CS = mVertexShader;
	lightCullPassDesc.pRootSignature = mRS;
	g_Device->CreateComputePipelineState(&lightCullPassDesc, IID_PPV_ARGS(&mPipelineState));
	mPipelineState->SetName(L"mLightCullPass");
}

void Renderer::LightCullPass::CreateRS()
{
	CD3DX12_ROOT_SIGNATURE_DESC lightCullRootSignatureDesc;
	D3D12_ROOT_PARAMETER lightBuffer = {};
	lightBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	lightBuffer.Descriptor.RegisterSpace = 0;
	lightBuffer.Descriptor.ShaderRegister = 1;
	lightBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_PARAMETER clusterBuffer = {};
	clusterBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	clusterBuffer.Descriptor.RegisterSpace = 0;
	clusterBuffer.Descriptor.ShaderRegister = 2;
	clusterBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_PARAMETER lightCullViewData = {};
	lightCullViewData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	lightCullViewData.Descriptor.RegisterSpace = 0;
	lightCullViewData.Descriptor.ShaderRegister = 0;
	lightCullViewData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	std::vector<D3D12_ROOT_PARAMETER> parameters =
	{
		lightCullViewData,lightBuffer,clusterBuffer
	};
	lightCullRootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ID3DBlob* signature;
	ID3DBlob* error;
	D3D12SerializeRootSignature(&lightCullRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRS));

}

void Renderer::LightCullPass::SetRenderPassStates(ID3D12GraphicsCommandList* InCmdList)
{
	mGraphicsCmd = InCmdList;
	mGraphicsCmd->SetPipelineState(mPipelineState);
	mGraphicsCmd->SetComputeRootSignature(mRS);
}

