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

void GAS::GameScene::CreateEntitiesWithMesh(const std::string InMeshFilePath) {
	//Todo: Thread Safe
    std::vector<ECS::StaticMesh> models;
    AssetLoader::ModelAssetLoader* loader = nullptr;
    auto extension = std::filesystem::path(InMeshFilePath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    if (extension == ".obj")
	{
        loader = AssetLoader::gObjModelLoader;
    } else if (extension == ".fbx") {
        loader = AssetLoader::gFbxModelLoader;
    }
    models = loader->LoadAssetFromFile(InMeshFilePath);
	Ensures(!models.empty());
	for (auto& mesh : models)
	{
        using namespace ECS;
        auto entity = mRegistery.create();
        mRegistery.emplace_or_replace<StaticMeshComponent>(entity, std::move(mesh));
        mRegistery.emplace_or_replace<TransformComponent>(entity, std::move(mesh));
	}
    mLoaded = true;
}

entt::registry& GAS::GameScene::GetRegistery()
{
	return mRegistery;
}

std::atomic_bool& GAS::GameScene::IsSceneReady() {
    return mLoaded;
}
