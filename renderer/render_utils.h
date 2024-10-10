#pragma once

namespace Renderer
{
	namespace Utils
	{
		D3D12_SHADER_BYTECODE ReadShader(std::string_view ShaderPath, std::string_view EntryPoint, std::string_view Type);
		SimpleMath::Vector3 GetLightGridZParams(float NearPlane, float FarPlane);
		SimpleMath::Vector4 CreateInvDeviceZToWorldZTransform(const SimpleMath::Matrix& ProjMatrix);
	}
}