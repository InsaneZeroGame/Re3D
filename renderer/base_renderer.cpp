#include "base_renderer.h"
#include "gui.h"

Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mContext(std::make_shared<RendererContext>(mCmdManager)),
	mBatchUploader(std::make_unique<ResourceUploadBatch>(g_Device)),
	mGraphicsFenceValue(1),
	mCurrentScene(nullptr)
{
	
}

Renderer::BaseRenderer::~BaseRenderer()
{

}

void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	mWindow = InWindow;
	mWidth = InWidth;
	mHeight = InHeight;
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	//Use Reverse Z
	mDefaultCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f, true);
	mDefaultCamera->LookAt({ 5.0,5.0,2.0 }, { 0.0f,0.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mShadowCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f, false);
	mShadowCamera->LookAt({ 5,25,0.0 }, { 0.0f,0.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mViewPort = { 0,0,(float)mWidth,(float)mHeight,0.0,1.0 };
	mRect = { 0,0,mWidth,mHeight };
}

void Renderer::BaseRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
	mCurrentScene = InGameScene;
	entt::registry& sceneRegistery = mCurrentScene->GetRegistery();
	mLoadResourceFuture = std::async(std::launch::async, [&]()
		{
			auto allStaticMeshComponents = sceneRegistery.view<ECS::StaticMeshComponent>();
			allStaticMeshComponents.each([this](auto entity, ECS::StaticMeshComponent& renderComponent) {
				mContext->LoadStaticMeshToGpu(renderComponent);
				});
			for (auto [textureName, textureData] : mCurrentScene->GetTextureMap())
			{
				LoadMaterial(textureName, textureData);
			}
		});
}

void Renderer::BaseRenderer::Update(float delta)
{

}

void Renderer::BaseRenderer::TransitState(ID3D12GraphicsCommandList* InCmd, ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter, UINT InSubResource /*= 0*/)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.Subresource = InSubResource;
	barrier.Transition.StateBefore = InBefore;
	barrier.Transition.StateAfter = InAfter;
	barrier.Transition.pResource = InResource;
	InCmd->ResourceBarrier(1, &barrier);
}

std::shared_ptr<Renderer::Resource::Texture> Renderer::BaseRenderer::LoadMaterial(std::string_view InTextureName, std::string_view InMatName /*= {}*/, const std::wstring& InDebugName /*= L""*/)
{
	auto newTextureData = AssetLoader::gStbTextureLoader->LoadTextureFromFile(InTextureName);
	if (!newTextureData.has_value())
	{
		return nullptr;
	}

	std::shared_ptr<Resource::Texture> newTexture = std::make_shared<Resource::Texture>();
	auto RowPitchBytes = newTextureData.value()->mWidth * newTextureData.value()->mComponent;
	newTexture->Create2D(RowPitchBytes, newTextureData.value()->mWidth, newTextureData.value()->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
	mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
	D3D12_SUBRESOURCE_DATA sourceData = {};
	sourceData.pData = newTextureData.value()->mdata;
	sourceData.RowPitch = RowPitchBytes;
	sourceData.SlicePitch = RowPitchBytes * newTextureData.value()->mHeight;
	mBatchUploader->Upload(newTexture->GetResource(), 0, &sourceData, 1);
	mBatchUploader->Transition(newTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
	newTexture->GetResource()->SetName(InDebugName.c_str());
	if (!InMatName.empty())
	{
		mTextureMap[std::string(InMatName)] = newTexture;
	}
	else
	{
		mTextureMap[std::filesystem::path(InTextureName).filename().string()] = newTexture;
	}
	return newTexture;
}

std::unordered_map<std::string, std::shared_ptr<Renderer::Resource::Texture>>& Renderer::BaseRenderer::GetSceneTextureMap()
{
	return mTextureMap;
}

std::shared_ptr<Renderer::RendererContext> Renderer::BaseRenderer::GetContext()
{
	return mContext;
}

void Renderer::BaseRenderer::CreateBuffers()
{
	for (auto i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		mFrameDataCPU[i] = std::make_shared<Resource::UploadBuffer>();
		mFrameDataCPU[i]->Create(L"FrameData", sizeof(FrameData));

		mFrameDataGPU[i] = std::make_unique<Resource::VertexBuffer>();
		mFrameDataGPU[i]->Create(L"FrameData", 1, sizeof(FrameData), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		mFrameData[i].DirectionalLightColor = SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0);
		mFrameData[i].DirectionalLightDir = SimpleMath::Vector3(1.0, 10.0, 12.0);
		mFrameData[i].SunLightIntensity = 1.0;

	}


	mLightUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	mLightUploadBuffer->Create(L"LightUploadBuffer", sizeof(ECS::LigthData) * mLights.size());

	float size = 65.0f;

	for (auto& light : mLights)
	{
		light.pos.x = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;
		light.pos.y = float(rand()) / RAND_MAX;
		light.pos.z = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;

		light.pos.x *= size;
		light.pos.y *= 15.0;
		light.pos.z *= size;
		light.pos.w = 1.0f;

		light.color.x = float(rand()) / RAND_MAX;
		light.color.y = float(rand()) / RAND_MAX;
		light.color.z = float(rand()) / RAND_MAX;
		light.radius_attenu.x = 55.0f;
		//light.radius_attenu.y = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu.z = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu.w = float(rand()) * 1.2f / RAND_MAX;
	}

	//mLights[0].pos = { 0.0, 1.0, 0.0, 1.0f };
	//mLights[0].radius_attenu = { 20.0, 0.0, 0.0, 1.0f };
	//mLights[0].color = {1.0f,0.0f,0.0f,1.0f};

	//mLights[1].pos = { -5.0, 0.0, 0.0, 1.0f };
	//mLights[1].radius_attenu = { 200.0, 0.0, 0.0, 1.0f };
	//mLights[1].color = { 0.0f,1.0f,0.0f,1.0f };
	//
	//mLights[3].pos = { 0.0, 0.0, 5.0, 1.0f };
	//mLights[3].radius_attenu = { 50.0, 0.0, 0.0, 1.0f };
	//mLights[3].color = { 1.0f,1.0f,0.0f,1.0f };
	//
	//mLights[2].pos = { 5.0, 0.0, 0.0, 1.0f };
	//mLights[2].radius_attenu = { 10.0, 0.0, 0.0, 1.0f };
	//mLights[2].color = { 1.0f,1.0f,1.0f,1.0f };


	//
	mLightUploadBuffer->UploadData<ECS::LigthData>(mLights);

	//mDummyBuffer = std::make_unique<Resource::StructuredBuffer>();
	//mDummyBuffer->Create(L"Dummy", 1, sizeof(uint8_t), nullptr);
	//mDummyBuffer->CreateSRV();

	//FrameResource Table
	mLightBuffer = std::make_unique<Resource::StructuredBuffer>();
	mLightBuffer->Create(L"LightBuffer", (UINT32)mLights.size(), sizeof(ECS::LigthData));

}

void Renderer::BaseRenderer::UpdataFrameData()
{
	auto frameDataCpuIndex = mFrameIndexCpu % SWAP_CHAIN_BUFFER_COUNT;

	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetNear(), mDefaultCamera->GetFar());
	mFrameData[frameDataCpuIndex].PrjView = mDefaultCamera->GetPrjView();
	mFrameData[frameDataCpuIndex].View = mDefaultCamera->GetView();
	mFrameData[frameDataCpuIndex].Prj = mDefaultCamera->GetPrj();
	mFrameData[frameDataCpuIndex].ShadowViewMatrix = mShadowCamera->GetView();
	mFrameData[frameDataCpuIndex].ShadowViewPrjMatrix = mShadowCamera->GetPrjView();
	mFrameData[frameDataCpuIndex].NormalMatrix = mDefaultCamera->GetNormalMatrix();
	mFrameData[frameDataCpuIndex].DirectionalLightDir.x = mSunLightDir[0];
	mFrameData[frameDataCpuIndex].DirectionalLightDir.y = mSunLightDir[1];
	mFrameData[frameDataCpuIndex].DirectionalLightDir.z = mSunLightDir[2];
	mFrameData[frameDataCpuIndex].SunLightIntensity = mSunLightIntensity;
	mFrameData[frameDataCpuIndex].DirectionalLightDir.Normalize();
	mFrameData[frameDataCpuIndex].LightGridZParams.x = gridParas.x;
	mFrameData[frameDataCpuIndex].LightGridZParams.y = gridParas.y;
	mFrameData[frameDataCpuIndex].LightGridZParams.z = gridParas.z;
	mFrameData[frameDataCpuIndex].ViewSizeAndInvSize = SimpleMath::Vector4((float)mWidth, (float)mHeight, 1.0f / (float)mWidth, 1.0f / (float)mHeight);
	mFrameData[frameDataCpuIndex].ClipToView = mDefaultCamera->GetClipToView();
	mFrameData[frameDataCpuIndex].ViewMatrix = mDefaultCamera->GetView();
	mFrameData[frameDataCpuIndex].InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
	mFrameDataCPU[frameDataCpuIndex]->UpdataData<FrameData>(mFrameData[frameDataCpuIndex]);
	//Advance CPU Frame Index
	mFrameIndexCpu++;
}

void Renderer::BaseRenderer::PrepairForRendering()
{
	mCmdManager->AllocateCmdAndFlush(D3D12_COMMAND_LIST_TYPE_DIRECT, [=](ID3D12GraphicsCommandList* lcmd)
		{
			TransitState(lcmd, mLightUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			TransitState(lcmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			lcmd->CopyResource(mLightBuffer->GetResource(), mLightUploadBuffer->GetResource());
			TransitState(lcmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		});
}

void Renderer::BaseRenderer::FirstFrame()
{
	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetFar(), mDefaultCamera->GetNear());
	mFrameData[0].LightGridZParams = SimpleMath::Vector4(gridParas.x, gridParas.y, gridParas.z, 0.0f);
	mFrameData[0].ClipToView = mDefaultCamera->GetClipToView();
	mFrameData[0].ViewMatrix = mDefaultCamera->GetView();
	mFrameData[0].InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));

}

std::shared_ptr<Renderer::Resource::Texture> Renderer::BaseRenderer::LoadMaterial(std::string_view InTextureName, AssetLoader::TextureData* textureData, const std::wstring& InDebugName /*= L""*/)
{
	if (mTextureMap.find(InTextureName.data()) != mTextureMap.end())
	{
		return mTextureMap[InTextureName.data()];
	}
	std::shared_ptr<Resource::Texture> newTexture = std::make_shared<Resource::Texture>();

	//Empty Data,load dds texture in renderer
	if (!textureData->mdata)
	{
		//load and upload
		newTexture->CreateDDSFromFile(textureData->mFilePath, mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT), false);
	}
	else
	{
		auto RowPitchBytes = textureData->mWidth * textureData->mComponent;
		newTexture->Create2D(RowPitchBytes, textureData->mWidth, textureData->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
		mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
		D3D12_SUBRESOURCE_DATA sourceData = {};
		sourceData.pData = textureData->mdata;
		sourceData.RowPitch = RowPitchBytes;
		sourceData.SlicePitch = RowPitchBytes * textureData->mHeight;
		mBatchUploader->Upload(newTexture->GetResource(), 0, &sourceData, 1);
		mBatchUploader->Transition(newTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
	}

	newTexture->GetResource()->SetName(InDebugName.c_str());
	if (!InTextureName.empty())
	{
		mTextureMap[std::string(InTextureName)] = newTexture;
	}
	else
	{
		mTextureMap[std::filesystem::path(InTextureName).filename().string()] = newTexture;
	}
	return newTexture;
}
