#pragma once

namespace Renderer
{
	namespace Resource
	{
		class Texture;
		class UploadBuffer;
		class StructuredBuffer;
		class VertexBuffer;
		class ColorBuffer;
	}

	struct FrameData
	{
		DirectX::SimpleMath::Matrix PrjView;
		DirectX::SimpleMath::Matrix View;
		DirectX::SimpleMath::Matrix Prj;
		DirectX::SimpleMath::Matrix NormalMatrix;
		DirectX::SimpleMath::Matrix ShadowViewMatrix;
		DirectX::SimpleMath::Matrix ShadowViewPrjMatrix;
		DirectX::SimpleMath::Vector3 DirectionalLightDir;
		float SunLightIntensity;
		DirectX::SimpleMath::Vector4 DirectionalLightColor;
		DirectX::SimpleMath::Vector4 ViewSizeAndInvSize;
		DirectX::SimpleMath::Matrix  ClipToView;
		DirectX::SimpleMath::Matrix  ViewMatrix;
		DirectX::SimpleMath::Vector4 LightGridZParams;
		DirectX::SimpleMath::Vector4 InvDeviceZToWorldZTransform;
	};

	struct OjbectData
	{
		DirectX::SimpleMath::Matrix ModelMatrix;
		DirectX::XMFLOAT3 DiffuseColor;
	};
}