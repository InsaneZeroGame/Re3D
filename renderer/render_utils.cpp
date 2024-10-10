#include "render_utils.h"
#include "utility.h"


LPCWSTR StringViewToLPCWSTR(std::string_view str) {
	// Convert std::string_view to std::wstring
	std::wstring wstr(str.begin(), str.end());

	// Allocate a buffer to hold the wide string
	static std::vector<wchar_t> buffer;
	buffer.assign(wstr.begin(), wstr.end());
	buffer.push_back(L'\0'); // Ensure null-termination

	// Return a pointer to the wide string
	return buffer.data();
}

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

std::string GetExecutablePath() {
	char buffer[MAX_PATH];
	DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
	if (length == 0) {
		// Handle error
		return "";
	}
	std::filesystem::path exePath(buffer);
	return exePath.parent_path().string();
}



D3D12_SHADER_BYTECODE Renderer::Utils::ReadShader(std::string_view ShaderPath,std::string_view EntryPoint,std::string_view Type)
{
	ID3DBlob* vertexShader;
	ID3DBlob* error;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

	//UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	auto path = GetExecutablePath() + "/shaders/" + std::string(ShaderPath);
	HRESULT hr = D3DCompileFromFile(StringViewToLPCWSTR(path), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.data(), Type.data(), compileFlags, 0, &vertexShader, &error);
	if (FAILED(hr)) {
		if (error) {
			// Print the error message to the log
			OutputDebugString(static_cast<const char*>(error->GetBufferPointer()));
			error->Release();
		}
		else {
			OutputDebugString(HrToString(hr).c_str());
		}
	}
	else {
		OutputDebugString("Shader compiled successfully.");
	}
	
	return CD3DX12_SHADER_BYTECODE(vertexShader);
}

SimpleMath::Vector3 Renderer::Utils::GetLightGridZParams(float NearPlane, float FarPlane)
{
	// S = distribution scale
	// B, O are solved for given the z distances of the first+last slice, and the # of slices.
	//
	// slice = log2(z*B + O) * S

	// Don't spend lots of resolution right in front of the near plane
	float NearOffset = .095f * 100;
	// Space out the slices so they aren't all clustered at the near plane
	float S = 4.05f;

	float N = NearPlane;
	float F = FarPlane;

	float O = (F - N * exp2((CLUSTER_Z - 1) / S)) / (F - N);
	float B = (1 - O) / N;

	return SimpleMath::Vector3(B, O, S);
}

SimpleMath::Vector4 Renderer::Utils::CreateInvDeviceZToWorldZTransform(const SimpleMath::Matrix& ProjMatrix)
{
	// The perspective depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	//
	// Note a reversed Z projection matrix will have A=0.
	//
	// Done in shader as:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	float DepthMul = (float)ProjMatrix.m[2][2];
	float DepthAdd = (float)ProjMatrix.m[3][2];

	if (DepthAdd == 0.f)
	{
		// Avoid dividing by 0 in this case
		DepthAdd = 0.00000001f;
	}

	// perspective
	// SceneDepth = 1.0f / (DeviceZ / ProjMatrix.M[3][2] - ProjMatrix.M[2][2] / ProjMatrix.M[3][2])

	// ortho
	// SceneDepth = DeviceZ / ProjMatrix.M[2][2] - ProjMatrix.M[3][2] / ProjMatrix.M[2][2];

	// combined equation in shader to handle either
	// SceneDepth = DeviceZ * View.InvDeviceZToWorldZTransform[0] + View.InvDeviceZToWorldZTransform[1] + 1.0f / (DeviceZ * View.InvDeviceZToWorldZTransform[2] - View.InvDeviceZToWorldZTransform[3]);

	// therefore perspective needs
	// View.InvDeviceZToWorldZTransform[0] = 0.0f
	// View.InvDeviceZToWorldZTransform[1] = 0.0f
	// View.InvDeviceZToWorldZTransform[2] = 1.0f / ProjMatrix.M[3][2]
	// View.InvDeviceZToWorldZTransform[3] = ProjMatrix.M[2][2] / ProjMatrix.M[3][2]

	// and ortho needs
	// View.InvDeviceZToWorldZTransform[0] = 1.0f / ProjMatrix.M[2][2]
	// View.InvDeviceZToWorldZTransform[1] = -ProjMatrix.M[3][2] / ProjMatrix.M[2][2] + 1.0f
	// View.InvDeviceZToWorldZTransform[2] = 0.0f
	// View.InvDeviceZToWorldZTransform[3] = 1.0f

	bool bIsPerspectiveProjection = ProjMatrix.m[3][3] < 1.0f;

	if (bIsPerspectiveProjection)
	{
		float SubtractValue = DepthMul / DepthAdd;

		// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
		// This fixes fog not being applied to the black background in the editor.
		SubtractValue -= 0.00000001f;

		return SimpleMath::Vector4(
			0.0f,
			0.0f,
			1.0f / DepthAdd,
			SubtractValue
		);
	}
	else
	{
		return SimpleMath::Vector4(
			(float)(1.0f / ProjMatrix.m[2][2]),
			(float)(-ProjMatrix.m[3][2] / ProjMatrix.m[2][2] + 1.0f),
			0.0f,
			1.0f
		);
	}
}
