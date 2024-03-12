#pragma once

namespace Renderer
{
	namespace Utils
	{
		D3D12_SHADER_BYTECODE ReadShader(_In_z_ const wchar_t* name);
		SimpleMath::Vector3 GetLightGridZParams(float NearPlane, float FarPlane);
		SimpleMath::Vector4 CreateInvDeviceZToWorldZTransform(const SimpleMath::Matrix& ProjMatrix);
	}
}