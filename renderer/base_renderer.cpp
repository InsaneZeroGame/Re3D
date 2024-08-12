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
	mDefaultCamera->LookAt({ 3.0,3.0,2.0 }, { 0.0f,3.0f,0.0f }, { 0.0f,1.0f,0.0f });
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
