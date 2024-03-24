#include "game_scene.h"
#include "asset_loader.h"
#include "obj_model_loader.h"

GAS::GameScene::GameScene():
	mRegistery({})
{
	

}

GAS::GameScene::~GameScene()
{

}

entt::entity GAS::GameScene::CreateEntityWithMesh(const std::string InMeshFilePath)
{
	//Todo: Thread Safe
	AssetLoader::ObjModelLoader* objLoader = AssetLoader::gObjModelLoader;
	auto model = objLoader->LoadAssetFromFile(InMeshFilePath);
	Ensures(model.has_value());
	auto modelValue = model.value();
	using namespace ECS;
	auto entity = mRegistery.create();
	mRegistery.emplace_or_replace<RenderComponent>(entity, modelValue);
	return entity;
}

entt::registry& GAS::GameScene::GetRegistery()
{
	return mRegistery;
}
