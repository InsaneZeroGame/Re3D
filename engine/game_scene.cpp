#include "game_scene.h"
#include "asset_loader.h"
#include "obj_model_loader.h"
#include "fbx_loader.h"
#include <algorithm>

GAS::GameScene::GameScene():
	mRegistery({})
{
	

}

GAS::GameScene::~GameScene()
{

}

std::vector<entt::entity> GAS::GameScene::CreateEntitiesWithMesh(const std::string InMeshFilePath) {
	//Todo: Thread Safe 
    std::vector<ECS::StaticMesh> models;
    AssetLoader::ModelAssetLoader* loader = nullptr;
    std::wstring extension = std::filesystem::path(InMeshFilePath).extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    if (extension == L".obj")
	{
        loader = AssetLoader::gObjModelLoader;
    } else if (extension == L".fbx") {
        loader = AssetLoader::gFbxModelLoader;
    }
    models = loader->LoadAssetFromFile(InMeshFilePath);
	mTextureMap = loader->GetTextureMap();
	Ensures(!models.empty());
    std::vector<entt::entity> res;
	for (auto& mesh : models)
	{
        using namespace ECS;
        auto entity = mRegistery.create();
        res.push_back(entity);
        mRegistery.emplace_or_replace<StaticMeshComponent>(entity, std::move(mesh));
        mRegistery.emplace_or_replace<TransformComponent>(entity, std::move(mesh));
	}
    mLoaded = true;
    for (auto ldelegate : sOnNewEntityAdded)
    {
        ldelegate(shared_from_this(), res);
    }
    return res;
}

entt::registry& GAS::GameScene::GetRegistery()
{
	return mRegistery;
}

std::atomic_bool& GAS::GameScene::IsSceneReady() {
    return mLoaded;
}

const std::unordered_map<std::string, AssetLoader::TextureData*>& GAS::GameScene::GetTextureMap()
{
    return mTextureMap;
}

void GAS::GameScene::SceneScale(float InScale)
{
    mScale = InScale;
    auto view = mRegistery.view<ECS::TransformComponent>();
    view.each([&](auto entity,auto& transformComponent) 
        {
            transformComponent.Scale(DirectX::SimpleMath::Vector3(mScale));
        });
}
